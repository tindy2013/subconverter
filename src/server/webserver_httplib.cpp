#include <string>
#ifdef MALLOC_TRIM
#include <malloc.h>
#endif // MALLOC_TRIM
#define CPPHTTPLIB_REQUEST_URI_MAX_LENGTH 16384
#include "httplib.h"

#include "utils/base64/base64.h"
#include "utils/logger.h"
#include "utils/string_hash.h"
#include "utils/stl_extra.h"
#include "utils/urlencode.h"
#include "webserver.h"

static const char *request_header_blacklist[] = {"host", "accept", "accept-encoding"};

static inline bool is_request_header_blacklisted(const std::string &header)
{
    for (auto &x : request_header_blacklist)
    {
        if (strcasecmp(x, header.c_str()) == 0)
        {
            return true;
        }
    }
    return false;
}

void WebServer::stop_web_server()
{
    SERVER_EXIT_FLAG = true;
}

static httplib::Server::Handler makeHandler(const responseRoute &rr)
{
    return [rr](const httplib::Request &request, httplib::Response &response)
    {
        Request req;
        Response resp;
        req.method = request.method;
        req.url = request.path;
        for (auto &h: request.headers)
        {
            if (startsWith(h.first, "LOCAL_")
            || startsWith(h.first, "REMOTE_")
            || is_request_header_blacklisted(h.first))
            {
                continue;
            }
            req.headers.emplace(h.first.data(), h.second.data());
        }
        req.argument = request.params;
        if (request.method == "POST" || request.method == "PUT" || request.method == "PATCH")
        {
            if (request.is_multipart_form_data() && !request.files.empty())
            {
                req.postdata = request.files.begin()->second.content;
            }
            else if (request.get_header_value("Content-Type") == "application/x-www-form-urlencoded")
            {
                req.postdata = urlDecode(request.body);
            }
            else
            {
                req.postdata = request.body;
            }
        }
        auto result = rr.rc(req, resp);
        response.status = resp.status_code;
        for (auto &h: resp.headers)
        {
            response.set_header(h.first, h.second);
        }
        auto content_type = resp.content_type;
        if (content_type.empty())
        {
            content_type = rr.content_type;
        }
        response.set_content(result, content_type);
    };
}

static std::string dump(const httplib::Headers &headers)
{
    std::string s;
    for (auto &x: headers)
    {
        if (startsWith(x.first, "LOCAL_") || startsWith(x.first, "REMOTE_"))
            continue;
        s += x.first + ": " + x.second + "|";
    }
    return s;
}

int WebServer::start_web_server_multi(listener_args *args)
{
    httplib::Server server;
    for (auto &x : responses)
    {
        switch (hash_(x.method))
        {
            case "GET"_hash: case "HEAD"_hash:
                server.Get(x.path, makeHandler(x));
                break;
            case "POST"_hash:
                server.Post(x.path, makeHandler(x));
                break;
            case "PUT"_hash:
                server.Put(x.path, makeHandler(x));
                break;
            case "DELETE"_hash:
                server.Delete(x.path, makeHandler(x));
                break;
            case "PATCH"_hash:
                server.Patch(x.path, makeHandler(x));
                break;
        }
    }
    server.Options(R"(.*)", [&](const httplib::Request &req, httplib::Response &res)
    {
        auto path = req.path;
        std::string allowed;
        for (auto &rr : responses)
        {
            if (rr.path == path)
            {
                allowed += rr.method + ",";
            }
        }
        if (!allowed.empty())
        {
            allowed.pop_back();
            res.status = 200;
            res.set_header("Access-Control-Allow-Methods", allowed);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Headers", "Content-Type,Authorization");
        }
        else
        {
            res.status = 404;
        }
    });
    server.set_pre_routing_handler([&](const httplib::Request &req, httplib::Response &res)
    {
        writeLog(0, "Accept connection from client " + req.remote_addr + ":" + std::to_string(req.remote_port), LOG_LEVEL_DEBUG);
        writeLog(0, "handle_cmd:    " + req.method + " handle_uri:    " + req.target, LOG_LEVEL_VERBOSE);
        writeLog(0, "handle_header: " + dump(req.headers), LOG_LEVEL_VERBOSE);

        if (req.has_header("SubConverter-Request"))
        {
            res.status = 500;
            res.set_content("Loop request detected!", "text/plain");
            return httplib::Server::HandlerResponse::Handled;
        }
        res.set_header("Server", "subconverter/" VERSION " cURL/" LIBCURL_VERSION);
        if (require_auth)
        {
            static std::string auth_token = "Basic " + base64Encode(auth_user + ":" + auth_password);
            auto auth = req.get_header_value("Authorization");
            if (auth != auth_token)
            {
                res.status = 401;
                res.set_header("WWW-Authenticate", "Basic realm=" + auth_realm + ", charset=\"UTF-8\"");
                res.set_content("Unauthorized", "text/plain");
                return httplib::Server::HandlerResponse::Handled;
            }
        }
        res.set_header("X-Client-IP", req.remote_addr);
        if (req.has_header("Access-Control-Request-Headers"))
        {
            res.set_header("Access-Control-Allow-Headers", req.get_header_value("Access-Control-Request-Headers"));
        }
        res.set_header("Access-Control-Allow-Origin", "*");
        return httplib::Server::HandlerResponse::Unhandled;
    });
    for (auto &x : redirect_map)
    {
        server.Get(x.first, [x](const httplib::Request &req, httplib::Response &res) {
            auto arguments = req.params;
            auto query = x.second;
            auto pos = query.find('?');
            query += pos == std::string::npos ? '?' : '&';
            for (auto &p: arguments)
            {
                query += p.first + "=" + urlEncode(p.second) + "&";
            }
            if (!query.empty())
            {
                query.pop_back();
            }
            res.set_redirect(query);
        });
    }
    server.set_exception_handler([](const httplib::Request &req, httplib::Response &res, const std::exception_ptr &e)
    {
        try
        {
            if (e) std::rethrow_exception(e);
        }
        catch (const httplib::Error &err)
        {
            res.set_content(to_string(err), "text/plain");
        }
        catch (const std::exception &ex)
        {
            std::string return_data = "Internal server error while processing request '" + req.target + "'!\n";
            return_data += "\n  exception: ";
            return_data += type(ex);
            return_data += "\n  what(): ";
            return_data += ex.what();
            res.status = 500;
            res.set_content(return_data, "text/plain");
        }
        catch (...)
        {
            res.status = 500;
        }
    });
    if (serve_file)
    {
        server.set_mount_point("/", serve_file_root);
    }
    server.new_task_queue = [args] {
        return new httplib::ThreadPool(args->max_workers);
    };
    server.bind_to_port(args->listen_address, args->port, 0);

    std::thread thread([&]()
    {
        server.listen_after_bind();
    });

    while (!SERVER_EXIT_FLAG)
    {
        if (args->looper_callback)
        {
            args->looper_callback();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(args->looper_interval));
    }

    server.stop();
    thread.join();
    return 0;
}

int WebServer::start_web_server(listener_args *args)
{
    return start_web_server_multi(args);
}
