#ifndef WEBSERVER_H_INCLUDED
#define WEBSERVER_H_INCLUDED

#include <string>
#include <map>

struct Request
{
    std::string method;
    std::string url;
    std::string argument;
    std::map<std::string, std::string> headers;
    std::string postdata;
};

struct Response
{
    int status_code = 200;
    std::string content_type;
    std::map<std::string, std::string> headers;
};

typedef std::string (*response_callback)(Request&, Response&); //process arguments and POST data and return served-content

#define RESPONSE_CALLBACK_ARGS Request &request, Response &response

struct listener_args
{
    std::string listen_address;
    int port;
    int max_conn;
    int max_workers;
};

void append_response(const std::string &method, const std::string &uri, const std::string &content_type, response_callback response);
void append_redirect(const std::string &uri, const std::string &target);
void reset_redirect();
int start_web_server(void *argv);
int start_web_server_multi(void *argv);
void stop_web_server();

#endif // WEBSERVER_H_INCLUDED
