#ifndef WEBSERVER_H_INCLUDED
#define WEBSERVER_H_INCLUDED

#include <string>
#include <map>
#include <atomic>
#include <curl/curlver.h>

#include "utils/map_extra.h"
#include "utils/string.h"
#include "version.h"

struct Request
{
    std::string method;
    std::string url;
    string_multimap argument;
    string_icase_map headers;
    std::string postdata;
};

struct Response
{
    int status_code = 200;
    std::string content_type;
    string_icase_map headers;
};

using response_callback = std::string (*)(Request&, Response&); //process arguments and POST data and return served-content

#define RESPONSE_CALLBACK_ARGS Request &request, Response &response

struct listener_args
{
    std::string listen_address;
    int port;
    int max_conn;
    int max_workers;
    void (*looper_callback)() = nullptr;
    uint32_t looper_interval = 200;
};

struct responseRoute
{
    std::string method;
    std::string path;
    std::string content_type;
    response_callback rc {};
};

class WebServer
{
public:
    std::string user_agent_str = "subconverter/" VERSION " cURL/" LIBCURL_VERSION;
    std::atomic_bool SERVER_EXIT_FLAG{false};

    // file server
    bool serve_file = false;
    std::string serve_file_root;

    // basic authentication
    bool require_auth = false;
    std::string auth_user, auth_password, auth_realm = "Please enter username and password:";

    void stop_web_server();

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
        std::map<std::string, std::string>().swap(redirect_map);
    }

    int start_web_server(listener_args *args);
    int start_web_server_multi(listener_args *args);

    std::vector<responseRoute> responses;
    string_map redirect_map;
};

#endif // WEBSERVER_H_INCLUDED
