#include <memory>
#include <cstdint>
#include <iostream>
#include <evhttp.h>

#include <string>
#include <vector>
#include <string.h>
#include <pthread.h>

#include "misc.h"
#include "webserver.h"
#include "socket.h"

struct responseRoute
{
    std::string method;
    std::string path;
    std::string content_type;
    response_callback rc;
};

std::vector<responseRoute> responses;

static inline int process_request(const char *method_str, std::string uri, std::string &postdata, std::string &content_type, std::string &return_data)
{
    std::string path, arguments;
    //std::cerr << "handle_cmd:    " << method_str << std::endl << "handle_uri:    " << uri << std::endl;

    if(strFind(uri, "?"))
    {
        path = uri.substr(0, uri.find("?"));
        arguments = uri.substr(uri.find("?") + 1);
    }
    else
        path = uri;

    for(responseRoute &x : responses)
    {
        if(strcmp(method_str, "OPTIONS") == 0 && x.method == postdata && x.path == path)
        {
            return 1;
        }
        else if(x.method.compare(method_str) == 0 && x.path == path)
        {
            response_callback &rc = x.rc;
            return_data = rc(arguments, postdata);
            content_type = x.content_type;
            return 0;
        }
    }

    return -1;
}

void OnReq(evhttp_request *req, void *args)
{
    const char *req_content_type = evhttp_find_header(req->input_headers, "Content-Type"), *req_ac_method = evhttp_find_header(req->input_headers, "Access-Control-Request-Method");
    const char *req_method = req_ac_method == NULL ? EVBUFFER_LENGTH(req->input_buffer) == 0 ? "GET" : "POST" : "OPTIONS", *uri = evhttp_request_get_uri(req);
    int retVal;
    std::string postdata, content_type, return_data;

    if(EVBUFFER_LENGTH(req->input_buffer) != 0)
    {
        postdata.assign((char *)EVBUFFER_DATA(req->input_buffer), EVBUFFER_LENGTH(req->input_buffer));
        if(req_content_type != NULL && strcmp(req_content_type, "application/x-www-form-urlencoded") == 0)
            postdata = UrlDecode(postdata);
    }
    else if(req_ac_method != NULL)
    {
        postdata.assign(req_ac_method);
    }

    retVal = process_request(req_method, uri, postdata, content_type, return_data);

    auto *OutBuf = evhttp_request_get_output_buffer(req);
    if (!OutBuf)
        return;

    switch(retVal)
    {
    case 1: //found OPTIONS
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Headers", "*");
        evhttp_send_reply(req, HTTP_OK, "", NULL);
        return;
        break;
    case 0: //found normal
        if(content_type.size())
        {
            if(content_type == "REDIRECT")
            {
                evhttp_add_header(req->output_headers, "Location", return_data.c_str());
                evhttp_send_reply(req, HTTP_MOVETEMP, "", NULL);
                return;
            }
            else
                evhttp_add_header(req->output_headers, "Content-Type", content_type.c_str());
        }
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");
        evhttp_add_header(req->output_headers, "Connection", "close");
        evbuffer_add(OutBuf, return_data.data(), return_data.size());
        evhttp_send_reply(req, HTTP_OK, "", OutBuf);
        break;
    case -1: //not found
        evhttp_send_error(req, HTTP_NOTFOUND, "Resource not found");
        break;
    default: //undefined behavior
        evhttp_send_error(req, HTTP_INTERNAL, "");
    }
}

int start_web_server(void *argv)
{
    struct listener_args *args = (listener_args*)argv;
    std::string listen_address = args->listen_address;
    int port = args->port;
    if (!event_init())
    {
        std::cerr << "Failed to init libevent." << std::endl;
        return -1;
    }
    const char *SrvAddress = listen_address.c_str();
    std::uint16_t SrvPort = port;
    std::unique_ptr<evhttp, decltype(&evhttp_free)> Server(evhttp_start(SrvAddress, SrvPort), &evhttp_free);
    if (!Server)
    {
        std::cerr << "Failed to init http server." << std::endl;
        return -1;
    }

    evhttp_set_allowed_methods(Server.get(), EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_OPTIONS);
    evhttp_set_gencb(Server.get(), OnReq, nullptr);
    if (event_dispatch() == -1)
    {
        std::cerr << "Failed to run message loop." << std::endl;
        return -1;
    }

    return 0;
}

void* httpserver_dispatch(void *arg)
{
    event_base_dispatch((struct event_base*)arg);
    return NULL;
}

int httpserver_bindsocket(std::string listen_address, int listen_port, int backlog)
{
    int ret;
    SOCKET nfd;
    nfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfd <= 0)
    {
        closesocket(nfd);
        return -1;
    }

    int one = 1;
    ret = setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(listen_address.data());
    addr.sin_port = htons(listen_port);

    ret = ::bind(nfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0)
    {
        closesocket(nfd);
        return -1;
    }
    ret = listen(nfd, backlog);
    if (ret < 0)
    {
        closesocket(nfd);
        return -1;
    }

    unsigned long ul = 1;
    ioctlsocket(nfd, FIONBIO, &ul); //set to non-blocking mode

    return nfd;
}

int start_web_server_multi(void *argv)
{
    struct listener_args *args = reinterpret_cast<listener_args*>(argv);
    std::string listen_address = args->listen_address;
    int port = args->port, nthreads = args->max_workers;
    int i, ret;

    int nfd = httpserver_bindsocket(listen_address, port, args->max_conn);
    if (nfd < 0)
        return -1;

    pthread_t ths[nthreads];
    for (i = 0; i < nthreads; i++)
    {
        struct event_base *base = event_init();
        if (base == NULL)
            return -1;
        struct evhttp *httpd = evhttp_new(base);
        if (httpd == NULL)
            return -1;
        ret = evhttp_accept_socket(httpd, nfd);
        if (ret != 0)
            return -1;

        evhttp_set_allowed_methods(httpd, EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_OPTIONS);
        evhttp_set_gencb(httpd, OnReq, nullptr);
        ret = pthread_create(&ths[i], NULL, httpserver_dispatch, base);
        if (ret != 0)
            return -1;
    }
    while(true)
        sleep(10000); //block forever

    return 0;
}

void append_response(std::string method, std::string uri, std::string content_type, response_callback response)
{
    responseRoute rr;
    rr.method = method;
    rr.path = uri;
    rr.content_type = content_type;
    rr.rc = response;
    responses.emplace_back(rr);
}
