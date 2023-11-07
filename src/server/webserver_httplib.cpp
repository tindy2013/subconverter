#include <string>
#ifdef MALLOC_TRIM
#include <malloc.h>
#endif // MALLOC_TRIM
#include "httplib.h"

#include "../utils/base64/base64.h"
#include "../utils/logger.h"
#include "../utils/string_hash.h"
#include "../utils/stl_extra.h"
#include "../utils/urlencode.h"
#include "webserver.h"

int WebServer::start_web_server(void *argv)
{
    return start_web_server_multi(argv);
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
            req.headers[h.first] = h.second;
        }
        for (auto &p: request.params)
        {
            req.argument += p.first + "=" + p.second + "&";
        }
        req.argument.pop_back();
        req.postdata = request.body;
        if (request.get_header_value("Content-Type") == "application/x-www-form-urlencoded")
        {
            req.postdata = urlDecode(req.postdata);
        }
        auto result = rr.rc(req, resp);
        response.status = resp.status_code;
        for (auto &h: resp.headers) {
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

int WebServer::start_web_server_multi(void *argv)
{
    httplib::Server server;
    auto *args = (listener_args *)argv;
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
    server.Options(R"(.*)", [&](const httplib::Request &req, httplib::Response &res) {
        auto path = req.path;
        std::string allowed;
        for (auto &rr : responses) {
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
    });
    server.set_pre_routing_handler([&](const httplib::Request &req, httplib::Response &res) {
        std::string params;
        for (auto &p: req.params)
        {
            params += p.first + "=" + p.second + "&";
        }
        if (!params.empty())
        {
            params.pop_back();
            params = "?" + params;
        }
        writeLog(0, "Accept connection from client " + req.remote_addr + ":" + std::to_string(req.remote_port), LOG_LEVEL_DEBUG);
        writeLog(0, "handle_cmd:    " + req.method + " handle_uri:    " + req.path + params, LOG_LEVEL_VERBOSE);

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
        return httplib::Server::HandlerResponse::Unhandled;
    });
    for (auto &x : redirect_map)
    {
        server.Get(x.first, [x](const httplib::Request &req, httplib::Response &res) {
            res.set_redirect(x.second);
        });
    }
    server.set_exception_handler([](const httplib::Request &req, httplib::Response &res, const std::exception_ptr &e) {
        try
        {
            std::rethrow_exception(e);
        }
        catch (const httplib::Error &err)
        {
            res.set_content(to_string(err), "text/plain");
        }
        catch (const std::exception &ex)
        {
            std::string params;
            for (auto &p: req.params)
            {
                params += p.first + "=" + p.second + "&";
            }
            params.pop_back();
            std::string return_data = "Internal server error while processing request path '" + req.path + "' with arguments '" + params + "'!\n";
            return_data += "\n  exception: ";
            return_data += type(ex);
            return_data += "\n  what(): ";
            return_data += ex.what();
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
    server.bind_to_port(args->listen_address, args->port, 0);

    pthread_t tid;
    pthread_create(&tid, nullptr, [](void *args) -> void * {
        auto *server = (httplib::Server *)args;
        server->listen_after_bind();
        return nullptr;
    }, &server);

    while (!SERVER_EXIT_FLAG)
    {
        if (args->looper_callback)
        {
            args->looper_callback();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(args->looper_interval));
    }

    server.stop();
    return 0;
}

void WebServer::append_response(const std::string &method, const std::string &uri, const std::string &content_type, response_callback response)
{
    responseRoute rr;
    rr.method = method;
    rr.path = uri;
    rr.content_type = content_type;
    rr.rc = response;
    responses.emplace_back(std::move(rr));
}

void WebServer::append_redirect(const std::string &uri, const std::string &target)
{
    redirect_map[uri] = target;
}

void WebServer::reset_redirect()
{
    eraseElements(redirect_map);
}

