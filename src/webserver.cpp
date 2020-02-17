#include <iostream>
#include <fstream>
#include <map>
#include <atomic>
#include <thread>
#include <pthread.h>

#include "misc.h"
#include "socket.h"
#include "webserver.h"

typedef std::vector<std::string> string_array;
int def_timeout = 5;

struct responseRoute
{
    std::string method;
    std::string path;
    std::string content_type;
    response_callback rc;
};

std::vector<responseRoute> responses;

//for use of multi-thread
int max_send_failure = 10;
std::atomic_bool SERVER_EXIT_FLAG(false);
std::atomic_int working_thread(0)

int sendall(SOCKET sock, std::string data)
{
    unsigned int total = 0, bytesleft = data.size();
    int sent = 0;
    const char* datastr = data.data();
    while(total < bytesleft)
    {
        sent = send(sock, datastr + total, bytesleft, 0);
        if(sent < 0)
        {
            std::cerr<<strerror(errno)<<std::endl;
            if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            {
                continue;
            }
            else
            {
                break;
            }
        }
        total += sent;
        bytesleft -= sent;
    }
    return sent == -1 ? -1 : 0;
}

void wrong_req(SOCKET sock)
{
    std::string response = "HTTP/1.1 501 Not Implemented\r\n"
                           "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\n"
                           "Content-Type: text/plain\r\n\r\n"
                           "The command is not yet completed\r\n";

    if (sendall(sock, response) == -1)
    {
        std::cerr << "Sending failed!" << std::endl;
    }
}

void file_not_found(std::string arguments, int sock)
{
    std::string prompt_info = "Not found:  " + arguments;
    std::string response = "HTTP/1.1 404 Not Found\r\n"
                           "Content-Type: text/plain\r\nConnection: close\r\n"
                           "Access-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\n"
                           "Content-Length: " + std::__cxx11::to_string(prompt_info.size()) + "\r\n\r\n" + prompt_info + "\r\n";

    if (sendall(sock, response) == -1)
    {
        printf("Sending error!");
        return;
    }
}

void send_header(SOCKET send_to, std::string content_type)
{
    std::string header = "HTTP/1.1 200 OK\r\nConnection: close\r\nCache-Control: no-cache, no-store, must-revalidate\r\nAccess-Control-Allow-Origin: *\r\n";
    if(content_type.size())
        header += "Content-Type: " + content_type + "\r\n";
    if(sendall(send_to, header) == -1)
    {
        printf("Sending error!");
    }
}

void send_options_header(SOCKET send_to)
{
    std::string header = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: *\r\n";
    if(sendall(send_to, header) == -1)
    {
        printf("Sending error!");
    }
}

char* file_type(const char* arg)
{
    char * temp;
    if ((temp=strrchr(arg,'.')) != NULL)
    {
        return temp+1;
    }
    return nullptr;
}

void serve_options(SOCKET sock)
{
    send_options_header(sock);
    std::string extra_header = "Content-Length: 0\r\n\r\n";
    sendall(sock, extra_header);
    sendall(sock, "\r\n\r\n");
}

void serve_content(SOCKET sock, std::string type, std::string content)
{
    send_header(sock, type.data());
    std::string extra_header = "Content-Length: " + std::__cxx11::to_string(content.size()) + "\r\n";
    sendall(sock, extra_header);
    send(sock, "\r\n", 2, 0);
    if (sendall(sock, content) == -1)
    {
        printf("Sending error!");
    }
    sendall(sock, "\r\n\r\n");
}

void send_file(std::string arguments, int sock)
{
    char* extension = file_type(arguments.data());
    std::string content_type = "text/plain", data;
    char sizestr[16] = {};
    int len;

    if (strcmp(extension, "html") == 0)
    {
        content_type = "text/html";
    }
    if (strcmp(extension, "gif") == 0)
    {
        content_type = "image/gif";
    }
    if (strcmp(extension, "jpg") == 0)
    {
        content_type = "image/jpg";
    }

    send_header(sock, content_type);
    sendall(sock, "Transfer-Encoding: chunked\r\n\r\n");
    data = fileGet(arguments);
    len = data.size();
    sprintf(sizestr, "%x\r\n", len);
    if (sendall(sock, sizestr) == -1)
    {
        printf("Sending error!");
    }
    if (sendall(sock, data) == -1)
    {
        printf("Sending error!");
    }
    len = 2;
    sendall(sock, "\r\n");

    len = 7;
    sendall(sock, "0\r\n\r\n");
}

int setTimeout(SOCKET s, int timeout)
{
    int ret = -1;
#ifdef _WIN32
    ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(int));
    ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));
#else
    struct timeval timeo = {timeout / 1000, (timeout % 1000) * 1000};
    ret = setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeo, sizeof(timeo));
    ret = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeo, sizeof(timeo));
#endif
    def_timeout = timeout;
    return ret;
}

void handle_req(std::string request, int client_sock)
{
    working_thread++;
    std::cerr<<"worker startup"<<std::endl;
    string_array vArray;
    char command[16] = {};
    char arguments[BUFSIZ] = {};
    std::string uri, args, target, postdata;

    if (sscanf(request.data(), "%s%s", command, arguments) != 2)
    {
        goto end;
    }

    std::cerr<<"handle_cmd:    "<<command<<"\n"<<"handle_path:   "<<arguments<<"\n";

    vArray = split(arguments, "?");

    if(vArray.size() > 2)
    {
        wrong_req(client_sock);
        goto end;
    }
    else if(vArray.size() > 1)
    {
        uri = vArray[0];
        args = vArray[1];
    }
    else
        uri = arguments;

    if(strcmp(command, "POST") == 0)
    {
        if(request.find("\r\n\r\n") != request.npos)
            postdata = request.substr(request.find("\r\n\r\n") + 4);
    }
    else if(strcmp(command, "OPTIONS") == 0)
    {
        serve_options(client_sock);
        goto end;
    }

    for(std::vector<responseRoute>::iterator iter = responses.begin(); iter != responses.end(); ++iter)
    {
        if(iter->method.compare(command) == 0 && iter->path == uri)
        {
            response_callback &rc = iter->rc;
            serve_content(client_sock, iter->content_type, rc(args, postdata));
            goto end;
        }
    }
    file_not_found(uri, client_sock);

end:
    std::cerr<<"worker stop"<<std::endl;
    sleep(1);
    closesocket(client_sock);
    working_thread--;
}

void append_response(std::string type, std::string request, std::string content_type, response_callback response)
{
    responseRoute rr;
    rr.method = type;
    rr.path = request;
    rr.content_type = content_type;
    rr.rc = response;
    responses.push_back(rr);
}

void stop_web_server()
{
    SERVER_EXIT_FLAG = true;
}

int start_web_server(void *argv)
{
    struct listener_args *args = (listener_args*)argv;
    args->max_workers = 1;
    return start_web_server_multi(args);
}

int start_web_server_multi(void *argv)
{
    //log startup
    struct listener_args *args = (listener_args*)argv;
    std::string listen_address = args->listen_address, request;
    int port = args->port, max_conn = args->max_conn, max_workers = args->max_workers, numbytes, worker_index = 0;
    socklen_t sock_size = sizeof(struct sockaddr_in);
    char buf[BUFSIZ];
    struct sockaddr_in user_socket, server_addr;
    SOCKET acc_socket;
    int server_socket, fail_counter = 0;
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    std::thread workers[max_workers];

    if (server_socket == -1)
    {
        //log socket error
        std::cerr<<"socket build error!"<<std::endl;
        return 0;
    }

    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((short)port);
    server_addr.sin_addr.s_addr = inet_addr(listen_address.data());
    if (::bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        //log bind error
        std::cerr<<"socket bind error!"<<std::endl;
        closesocket(server_socket);
        return 0;
    }
    if (listen(server_socket, max_conn) == -1 )
    {
        //log listen error
        std::cerr<<"socket listen error!"<<std::endl;
        closesocket(server_socket);
        return 0;
    }
    setTimeout(server_socket, 500);

    while(true)
    {
        acc_socket = accept(server_socket, (struct sockaddr *)&user_socket, &sock_size); //wait for connection
        if(acc_socket < 0)
        {
            if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            {
                fail_counter++;
                if(fail_counter > max_send_failure)
                    break;
                continue;
            }
            else
            {
                break;
            }
        }
        request = "";
        while(true) //receive the complete request
        {
            numbytes = recv(acc_socket, buf, BUFSIZ - 1, 0);
            if(numbytes > 0) //received
                request.append(buf);
            if(numbytes < 0)
            {
                if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            if(numbytes == 0)
                break;
        }

        //handle_req(buf, acc_socket);

        while(working_thread >= max_workers)
        {
            sleep(10);
            if(SERVER_EXIT_FLAG)
                break;
        }
        while(workers[worker_index].get_id() != std::thread::id())
        {
            worker_index++;
        }
        workers[worker_index] = std::thread(handle_req, request, acc_socket);
        workers[worker_index].detach();
        worker_index++;
        if(worker_index > max_workers)
            worker_index = 0;
    }
    return 0;
}
