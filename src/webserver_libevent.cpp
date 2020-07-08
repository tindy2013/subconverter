#include <memory>
#include <cstdint>
#include <iostream>
#include <evhttp.h>
#include <atomic>
#ifdef MALLOC_TRIM
#include <malloc.h>
#endif // MALLOC_TRIM

#include <string>
#include <vector>
#include <map>
#include <string.h>
#include <pthread.h>

#include "misc.h"
#include "webserver.h"
#include "socket.h"
#include "logger.h"

extern std::string user_agent_str;
std::atomic_bool SERVER_EXIT_FLAG(false);

struct responseRoute
{
    std::string method;
    std::string path;
    std::string content_type;
    response_callback rc;
};

std::vector<responseRoute> responses;
string_map redirect_map;

const char *request_header_blacklist[] = {"host", "user-agent", "accept", "accept-encoding"};

static inline void buffer_cleanup(struct evbuffer *eb)
{
    //evbuffer_free(eb);
#ifdef MALLOC_TRIM
    malloc_trim(0);
#endif // MALLOC_TRIM
}

static inline int process_request(Request &request, Response &response, std::string &return_data)
{
    writeLog(0, "handle_cmd:    " + request.method + " handle_uri:    " + request.url, LOG_LEVEL_VERBOSE);

    string_size pos = request.url.find("?");
    if(pos != request.url.npos)
    {
        request.argument = request.url.substr(pos + 1);
        request.url.erase(pos);
    }

    for(responseRoute &x : responses)
    {
        if(request.method == "OPTIONS" && request.postdata.find(x.method) != 0 && x.path == request.url)
        {
            return 1;
        }
        else if(x.method == request.method && x.path == request.url)
        {
            response_callback &rc = x.rc;
            return_data = rc(request, response);
            response.content_type = x.content_type;
            return 0;
        }
    }

    auto iter = redirect_map.find(request.url);
    if(iter != redirect_map.end())
    {
        return_data = iter->second;
        if(request.argument.size())
        {
            if(return_data.find("?") != return_data.npos)
                return_data += "&" + request.argument;
            else
                return_data += "?" + request.argument;
        }
        response.content_type = "REDIRECT";
        return 0;
    }

    return -1;
}

void OnReq(evhttp_request *req, void *args)
{
    const char *req_content_type = evhttp_find_header(req->input_headers, "Content-Type"), *req_ac_method = evhttp_find_header(req->input_headers, "Access-Control-Request-Method");
    const char *req_method = req_ac_method == NULL ? EVBUFFER_LENGTH(req->input_buffer) == 0 ? "GET" : "POST" : "OPTIONS", *uri = req->uri;
    const char *user_agent = evhttp_find_header(req->input_headers, "User-Agent");
    char *client_ip;
    u_short client_port;
    evhttp_connection_get_peer(evhttp_request_get_connection(req), &client_ip, &client_port);
    //std::cerr<<"Accept connection from client "<<client_ip<<":"<<client_port<<"\n";
    writeLog(0, "Accept connection from client " + std::string(client_ip) + ":" + std::to_string(client_port), LOG_LEVEL_DEBUG);
    int retVal;
    std::string postdata, content_type, return_data;

    if(user_agent != NULL && user_agent_str.compare(user_agent) == 0)
    {
        evhttp_send_error(req, 500, "Loop request detected!");
        return;
    }

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

    Request request;
    Response response;
    request.method = req_method;
    request.url = uri;

    struct evkeyval* kv = req->input_headers->tqh_first;
    while (kv)
    {
        if(std::none_of(std::begin(request_header_blacklist), std::end(request_header_blacklist), [&](auto x){ return strcasecmp(kv->key, x) == 0; }))
            request.headers.emplace(kv->key, kv->value);
        kv = kv->next.tqe_next;
    }
    if(user_agent)
        request.headers.emplace("X-User-Agent", user_agent);
    request.headers.emplace("X-Client-IP", client_ip);

    retVal = process_request(request, response, return_data);
    content_type = response.content_type;

    auto *OutBuf = evhttp_request_get_output_buffer(req);
    //struct evbuffer *OutBuf = evbuffer_new();
    if (!OutBuf)
        return;

    for(auto &x : response.headers)
        evhttp_add_header(req->output_headers, x.first.data(), x.second.data());

    switch(retVal)
    {
    case 1: //found OPTIONS
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Headers", "*");
        evhttp_send_reply(req, response.status_code, "", NULL);
        break;
    case 0: //found normal
        if(content_type.size())
        {
            if(content_type == "REDIRECT")
            {
                evhttp_add_header(req->output_headers, "Location", return_data.c_str());
                evhttp_send_reply(req, HTTP_MOVETEMP, "", NULL);
                buffer_cleanup(OutBuf);
                return;
            }
            else
                evhttp_add_header(req->output_headers, "Content-Type", content_type.c_str());
        }
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");
        evhttp_add_header(req->output_headers, "Connection", "close");
        evbuffer_add(OutBuf, return_data.data(), return_data.size());
        evhttp_send_reply(req, response.status_code, "", OutBuf);
        break;
    case -1: //not found
        return_data = "File not found.";
        evbuffer_add(OutBuf, return_data.data(), return_data.size());
        evhttp_send_reply(req, HTTP_NOTFOUND, "", OutBuf);
        //evhttp_send_error(req, HTTP_NOTFOUND, "Resource not found");
        break;
    default: //undefined behavior
        evhttp_send_error(req, HTTP_INTERNAL, "");
    }
    buffer_cleanup(OutBuf);
}

int start_web_server(void *argv)
{
    struct listener_args *args = reinterpret_cast<listener_args*>(argv);
    std::string listen_address = args->listen_address;
    int port = args->port;
    if (!event_init())
    {
        //std::cerr << "Failed to init libevent." << std::endl;
        writeLog(0, "Failed to init libevent.", LOG_LEVEL_FATAL);
        return -1;
    }
    const char *SrvAddress = listen_address.c_str();
    std::uint16_t SrvPort = port;
    std::unique_ptr<evhttp, decltype(&evhttp_free)> Server(evhttp_start(SrvAddress, SrvPort), &evhttp_free);
    if (!Server)
    {
        //std::cerr << "Failed to init http server." << std::endl;
        writeLog(0, "Failed to init http server.", LOG_LEVEL_FATAL);
        return -1;
    }

    evhttp_set_allowed_methods(Server.get(), EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_OPTIONS);
    evhttp_set_gencb(Server.get(), OnReq, nullptr);
    evhttp_set_timeout(Server.get(), 30);
    if (event_dispatch() == -1)
    {
        //std::cerr << "Failed to run message loop." << std::endl;
        writeLog(0, "Failed to run message loop.", LOG_LEVEL_FATAL);
        return -1;
    }

    return 0;
}

void* httpserver_dispatch(void *arg)
{
    event_base_dispatch(reinterpret_cast<event_base*>(arg));
    event_base_free(reinterpret_cast<event_base*>(arg)); //free resources
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
#ifdef SO_NOSIGPIPE
    ret = setsockopt(nfd, SOL_SOCKET, SO_NOSIGPIPE, (char *)&one, sizeof(int));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(listen_address.data());
    addr.sin_port = htons(listen_port);

    ret = ::bind(nfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
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
    struct event_base *base[nthreads];
    for (i = 0; i < nthreads; i++)
    {
        base[i] = event_init();
        if (base[i] == NULL)
            return -1;
        struct evhttp *httpd = evhttp_new(base[i]);
        if (httpd == NULL)
            return -1;
        ret = evhttp_accept_socket(httpd, nfd);
        if (ret != 0)
            return -1;

        evhttp_set_allowed_methods(httpd, EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_OPTIONS);
        evhttp_set_gencb(httpd, OnReq, nullptr);
        evhttp_set_timeout(httpd, 30);
        ret = pthread_create(&ths[i], NULL, httpserver_dispatch, base[i]);
        if (ret != 0)
            return -1;
    }
    while(!SERVER_EXIT_FLAG)
        sleep(200); //block forever until receive stop signal

    for (i = 0; i < nthreads; i++)
        event_base_loopbreak(base[i]); //stop the loop

    shutdown(nfd, SD_BOTH); //stop accept call
    closesocket(nfd); //close listener socket

    return 0;
}

void stop_web_server()
{
    SERVER_EXIT_FLAG = true;
}

void append_response(const std::string &method, const std::string &uri, const std::string &content_type, response_callback response)
{
    responseRoute rr;
    rr.method = method;
    rr.path = uri;
    rr.content_type = content_type;
    rr.rc = response;
    responses.emplace_back(std::move(rr));
}

void append_redirect(const std::string &uri, const std::string &target)
{
    redirect_map[uri] = target;
}

void reset_redirect()
{
    eraseElements(redirect_map);
}
