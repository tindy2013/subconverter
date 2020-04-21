#ifndef WEBSERVER_H_INCLUDED
#define WEBSERVER_H_INCLUDED

typedef std::string (*response_callback)(std::string, std::string, int*, std::map<std::string, std::string>&); //process arguments and POST data and return served-content

#define RESPONSE_CALLBACK_ARGS std::string argument, std::string postdata, int *status_code, std::map<std::string, std::string> &extra_headers

struct listener_args
{
    std::string listen_address;
    int port;
    int max_conn;
    int max_workers;
};

void append_response(std::string method, std::string uri, std::string content_type, response_callback response);
void append_redirect(std::string uri, std::string target);
void reset_redirect();
int start_web_server(void *argv);
int start_web_server_multi(void *argv);
void stop_web_server();

#endif // WEBSERVER_H_INCLUDED
