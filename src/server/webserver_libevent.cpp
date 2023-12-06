#include <memory>
#include <cstdint>
#include <evhttp.h>
#include <atomic>
#ifdef MALLOC_TRIM
#include <malloc.h>
#endif // MALLOC_TRIM

#include <string>
#include <map>
#include <algorithm>
#include <cstring>
#include <pthread.h>
#include <thread>

#include "utils/base64/base64.h"
#include "utils/file_extra.h"
#include "utils/logger.h"
#include "utils/stl_extra.h"
#include "utils/string.h"
#include "utils/urlencode.h"
#include "socket.h"
#include "webserver.h"

template <typename Lambda, class Ret, class... Args, class Pointer = Ret (*)(Args...)>
Pointer deduced_wrap(
    const std::function<Ret(Args...)> &func)
{
    static auto saved = func;
    static Pointer p = [](Args... args) {
        return saved(std::forward<Args>(args)...);
    };
    return p;
}

template <typename Lambda>
auto *wrap(Lambda &&func)
{
    return deduced_wrap<Lambda>(std::function{func});
}

struct MIME_type
{
    std::string extension;
    std::string mimetype;
};

MIME_type mime_types[] = {{"html htm shtml","text/html"},
                          {"css",           "text/css"},
                          {"jpeg jpg",      "image/jpeg"},
                          {"js",            "application/javascript"},
                          {"txt",           "text/plain"},
                          {"png",           "image/png"},
                          {"ico",           "image/x-icon"},
                          {"svg svgz",      "image/svg+xml"},
                          {"woff",          "application/font-woff"},
                          {"json",          "application/json"}};

bool matchSpaceSeparatedList(const std::string& source, const std::string &target)
{
    string_size pos_begin = 0, pos_end, total = source.size();
    while(pos_begin < total)
    {
        pos_end = source.find(' ', pos_begin);
        if(pos_end == std::string::npos)
            pos_end = total;
        if(source.compare(pos_begin, pos_end - pos_begin, target) == 0)
            return true;
        pos_begin = pos_end + 1;
    }
    return false;
}

std::string checkMIMEType(const std::string &filename)
{
    string_size name_begin, name_end;
    name_begin = filename.rfind('/');
    if(name_begin == std::string::npos)
        name_begin = 0;
    name_end = filename.rfind('.');
    if(name_end == std::string::npos || name_end < name_begin || name_end == filename.size() - 1)
        return "application/octet-stream";
    std::string extension = filename.substr(name_end + 1);
    for(MIME_type &x : mime_types)
        if(matchSpaceSeparatedList(x.extension, extension))
            return x.mimetype;
    return "application/octet-stream";
}

int serveFile(WebServer *server, const std::string &filename, std::string &content_type, std::string &return_data)
{
    std::string realname = server->serve_file_root + filename;
    if(filename == "/")
        realname += "index.html";
    if(!fileExist(realname))
        return 1;

    return_data = fileGet(realname, false);
    content_type = checkMIMEType(realname);
    writeLog(0, "file-server: serving '" + filename + "' type '" + content_type + "'", LOG_LEVEL_INFO);
    return 0;
}

const char *request_header_blacklist[] = {"host", "accept", "accept-encoding"};

static inline void buffer_cleanup(struct evbuffer *eb)
{
    (void)eb;
    //evbuffer_free(eb);
#ifdef MALLOC_TRIM
    malloc_trim(0);
#endif // MALLOC_TRIM
}

static int process_request(WebServer *server, Request &request, Response &response, std::string &return_data)
{
    writeLog(0, "handle_cmd:    " + request.method + " handle_uri:    " + request.url, LOG_LEVEL_VERBOSE);

    string_size pos = request.url.find('?');
    if(pos != std::string::npos)
    {
        auto argument = split(request.url.substr(pos + 1), "&");
        for(auto &x : argument)
        {
            string_size pos2 = x.find('=');
            if(pos2 != std::string::npos)
                request.argument.emplace(x.substr(0, pos2), x.substr(pos2 + 1));
            else
                request.argument.emplace(x, "");
        }
        request.url.erase(pos);
    }

    if(request.method == "OPTIONS")
    {
        for(responseRoute &x : server->responses)
            if(matchSpaceSeparatedList(replaceAllDistinct(request.postdata, ",", ""), x.method) && x.path == request.url)
                return 1;
        return -1;
    }

    for(responseRoute &x : server->responses)
    {
        if(x.method == request.method && x.path == request.url)
        {
            response_callback &rc = x.rc;
            try
            {
                return_data = rc(request, response);
                response.content_type = x.content_type;
            }
            catch(std::exception &e)
            {
                return_data = "Internal server error while processing request path '" + request.url + "' with arguments '" + joinArguments(request.argument) + "'!";
                return_data += "\n  exception: ";
                return_data += type(e);
                return_data += "\n  what(): ";
                return_data += e.what();
                response.content_type = "text/plain";
                response.status_code = 500;
                writeLog(0, return_data, LOG_LEVEL_ERROR);
            }
            return 0;
        }
    }

    auto iter = server->redirect_map.find(request.url);
    if(iter != server->redirect_map.end())
    {
        return_data = iter->second;
        if(!request.argument.empty())
        {
            if(return_data.find('?') != std::string::npos)
                return_data += "&" + joinArguments(request.argument);
            else
                return_data += "?" + joinArguments(request.argument);
        }
        return 2;
    }

    if(server->serve_file)
    {
        if(request.method == "GET" && serveFile(server, request.url, response.content_type, return_data) == 0)
            return 0;
    }

    return -1;
}

static void on_request(evhttp_request *req, void *args)
{
    auto server = (WebServer*) args;
    static std::string auth_token = "Basic " + base64Encode(server->auth_user + ":" + server->auth_password);
    const char *req_content_type = evhttp_find_header(req->input_headers, "Content-Type"), *req_ac_method = evhttp_find_header(req->input_headers, "Access-Control-Request-Method");
    const char *uri = req->uri, *internal_flag = evhttp_find_header(req->input_headers, "SubConverter-Request");

    char *client_ip;
    u_short client_port;
    evhttp_connection_get_peer(evhttp_request_get_connection(req), &client_ip, &client_port);
    //std::cerr<<"Accept connection from client "<<client_ip<<":"<<client_port<<"\n";
    writeLog(0, "Accept connection from client " + std::string(client_ip) + ":" + std::to_string(client_port), LOG_LEVEL_DEBUG);

    if (internal_flag != nullptr)
    {
        evhttp_send_error(req, 500, "Loop request detected!");
        return;
    }

    if (server->require_auth)
    {
        const char *auth = evhttp_find_header(req->input_headers, "Authorization");
        if (auth == nullptr || auth_token != auth)
        {
            evhttp_add_header(req->output_headers, "WWW-Authenticate", ("Basic realm=\"" + server->auth_realm + "\"").data());
            auto buffer = evhttp_request_get_output_buffer(req);
            evbuffer_add_printf(buffer, "Unauthorized");
            evhttp_send_reply(req, 401, nullptr, buffer);
            buffer_cleanup(buffer);
            return;
        }
    }

    Request request;
    Response response;
    size_t buffer_len = evbuffer_get_length(req->input_buffer);
    if (buffer_len != 0)
    {
        request.postdata.assign(reinterpret_cast<char*>(evbuffer_pullup(req->input_buffer, -1)), buffer_len);
        if(req_content_type != nullptr && strcmp(req_content_type, "application/x-www-form-urlencoded") == 0)
            request.postdata = urlDecode(request.postdata);
    }
    else if (req_ac_method != nullptr)
    {
        request.postdata.assign(req_ac_method);
    }

    switch (req->type)
    {
        case EVHTTP_REQ_GET: request.method = "GET"; break;
        case EVHTTP_REQ_POST: request.method = "POST"; break;
        case EVHTTP_REQ_OPTIONS: request.method = "OPTIONS"; break;
        case EVHTTP_REQ_PUT: request.method = "PUT"; break;
        case EVHTTP_REQ_PATCH: request.method = "PATCH"; break;
        case EVHTTP_REQ_DELETE: request.method = "DELETE"; break;
        case EVHTTP_REQ_HEAD: request.method = "HEAD"; break;
        default: break;
    }
    request.url = uri;

    struct evkeyval* kv = req->input_headers->tqh_first;
    while (kv)
    {
        if(std::none_of(std::begin(request_header_blacklist), std::end(request_header_blacklist), [&](auto x){ return strcasecmp(kv->key, x) == 0; }))
            request.headers.emplace(kv->key, kv->value);
        kv = kv->next.tqe_next;
    }
    request.headers.emplace("X-Client-IP", client_ip);

    std::string return_data;
    int retVal = process_request(server, request, response, return_data);
    std::string &content_type = response.content_type;

    auto *output_buffer = evhttp_request_get_output_buffer(req);
    if (!output_buffer)
    {
        evhttp_send_error(req, HTTP_INTERNAL, nullptr);
        return;
    }

    for (auto &x : response.headers)
        evhttp_add_header(req->output_headers, x.first.data(), x.second.data());

    switch (retVal)
    {
    case 1: //found OPTIONS
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Headers", "*");
        evhttp_send_reply(req, response.status_code, nullptr, nullptr);
        break;
    case 2: //found redirect
        evhttp_add_header(req->output_headers, "Location", return_data.c_str());
        evhttp_send_reply(req, HTTP_MOVETEMP, nullptr, nullptr);
        break;
    case 0: //found normal
        if (!content_type.empty())
            evhttp_add_header(req->output_headers, "Content-Type", content_type.c_str());
        evhttp_add_header(req->output_headers, "Access-Control-Allow-Origin", "*");
        evhttp_add_header(req->output_headers, "Connection", "close");
        evbuffer_add(output_buffer, return_data.data(), return_data.size());
        evhttp_send_reply(req, response.status_code, nullptr, output_buffer);
        break;
    case -1: //not found
        return_data = "File not found.";
        evbuffer_add(output_buffer, return_data.data(), return_data.size());
        evhttp_send_reply(req, HTTP_NOTFOUND, nullptr, output_buffer);
        //evhttp_send_error(req, HTTP_NOTFOUND, "Resource not found");
        break;
    default: //undefined behavior
        evhttp_send_error(req, HTTP_INTERNAL, nullptr);
    }
    buffer_cleanup(output_buffer);
}

int WebServer::start_web_server(listener_args *args)
{
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
    std::unique_ptr<evhttp, decltype(&evhttp_free)> server(evhttp_start(SrvAddress, SrvPort), &evhttp_free);
    if (!server)
    {
        //std::cerr << "Failed to init http server." << std::endl;
        writeLog(0, "Failed to init http server.", LOG_LEVEL_FATAL);
        return -1;
    }

    evhttp_set_allowed_methods(server.get(), EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_OPTIONS | EVHTTP_REQ_PUT | EVHTTP_REQ_PATCH | EVHTTP_REQ_DELETE | EVHTTP_REQ_HEAD);
    evhttp_set_gencb(server.get(), on_request, this);
    evhttp_set_timeout(server.get(), 30);
    if (event_dispatch() == -1)
    {
        //std::cerr << "Failed to run message loop." << std::endl;
        writeLog(0, "Failed to run message loop.", LOG_LEVEL_FATAL);
        return -1;
    }

    return 0;
}

static void* httpserver_dispatch(void *arg)
{
    event_base_dispatch(reinterpret_cast<event_base*>(arg));
    event_base_free(reinterpret_cast<event_base*>(arg)); //free resources
    return nullptr;
}

static int httpserver_bindsocket(std::string listen_address, int listen_port, int backlog)
{
    SOCKET nfd;
    nfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfd <= 0)
        return -1;

    int one = 1;
    if (setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int)) < 0)
    {
        closesocket(nfd);
        return -1;
    }
#ifdef SO_NOSIGPIPE
    if (setsockopt(nfd, SOL_SOCKET, SO_NOSIGPIPE, (char *)&one, sizeof(int)) < 0)
    {
        closesocket(nfd);
        return -1;
    }
#endif

    struct sockaddr_in addr {};
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(listen_address.data());
    addr.sin_port = htons(listen_port);

    if (::bind(nfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0 || listen(nfd, backlog) < 0)
    {
        closesocket(nfd);
        return -1;
    }

    unsigned long ul = 1;
    ioctlsocket(nfd, FIONBIO, &ul); //set to non-blocking mode

    return nfd;
}

int WebServer::start_web_server_multi(listener_args *args)
{
    std::string listen_address = args->listen_address;
    int port = args->port, nthreads = args->max_workers, max_conn = args->max_conn;

    int nfd = httpserver_bindsocket(listen_address, port, max_conn);
    if (nfd < 0)
        return -1;

    pthread_t ths[nthreads];
    struct event_base *base[nthreads];
    for (int i = 0; i < nthreads; i++)
    {
        base[i] = event_init();
        if (base[i] == nullptr)
            return -1;
        struct evhttp *httpd = evhttp_new(base[i]);
        if (httpd == nullptr)
            return -1;
        if (evhttp_accept_socket(httpd, nfd) != 0)
            return -1;

        evhttp_set_allowed_methods(httpd, EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_OPTIONS | EVHTTP_REQ_PUT | EVHTTP_REQ_PATCH | EVHTTP_REQ_DELETE | EVHTTP_REQ_HEAD);
        evhttp_set_gencb(httpd, on_request, this);
        evhttp_set_timeout(httpd, 30);
        if (pthread_create(&ths[i], nullptr, httpserver_dispatch, base[i]) != 0)
            return -1;
    }
    while (!SERVER_EXIT_FLAG)
    {
        if (args->looper_callback != nullptr)
            args->looper_callback();
        std::this_thread::sleep_for(std::chrono::milliseconds(args->looper_interval)); //block forever until receive stop signal
    }

    for (int i = 0; i < nthreads; i++)
        event_base_loopbreak(base[i]); //stop the loop

    shutdown(nfd, SD_BOTH); //stop accept call
    closesocket(nfd); //close listener socket

    return 0;
}

void WebServer::stop_web_server()
{
    SERVER_EXIT_FLAG = true;
}
