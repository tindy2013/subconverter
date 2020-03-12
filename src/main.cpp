#include <iostream>
#include <string>
#include <unistd.h>
#include <signal.h>

#include "interfaces.h"
#include "version.h"
#include "misc.h"
#include "socket.h"
#include "webget.h"
#include "logger.h"

extern std::string pref_path, access_token, listen_address, gen_profile;
extern bool api_mode, generator_mode, cfw_child_process, update_ruleset_on_request;
extern int listen_port, max_concurrent_threads, max_pending_connections;
extern string_array rulesets;
extern std::vector<ruleset_content> ruleset_content_array;

#ifndef _WIN32
void SetConsoleTitle(std::string title)
{
    system(std::string("echo \"\\033]0;" + title + "\\007\\c\"").data());
}
#endif // _WIN32

void setcd(std::string &file)
{
    char szTemp[1024] = {}, filename[256] = {};
    std::string path;
#ifdef _WIN32
    char *pname = NULL;
    DWORD retVal = GetFullPathName(file.data(), 1023, szTemp, &pname);
    if(!retVal)
        return;
    strcpy(filename, pname);
    strrchr(szTemp, '\\')[1] = '\0';
#else
    char *ret = realpath(file.data(), szTemp);
    if(ret == NULL)
        return;
    ret = strcpy(filename, strrchr(szTemp, '/') + 1);
    if(ret == NULL)
        return;
    strrchr(szTemp, '/')[1] = '\0';
#endif // _WIN32
    file.assign(filename);
    path.assign(szTemp);
    chdir(path.data());
}

void chkArg(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-cfw") == 0)
        {
            cfw_child_process = true;
            update_ruleset_on_request = true;
        }
        else if(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0)
            pref_path.assign(argv[++i]);
        else if(strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gen") == 0)
            generator_mode = true;
        else if(strcmp(argv[i], "--artifact") == 0)
            gen_profile.assign(argv[++i]);
    }
}

void signal_handler(int sig)
{
    //std::cerr<<"Interrupt signal "<<sig<<" received. Exiting gracefully...\n";
    writeLog(0, "Interrupt signal " + std::to_string(sig) + " received. Exiting gracefully...", LOG_LEVEL_FATAL);
    switch(sig)
    {
#ifndef _WIN32
    case SIGHUP:
    case SIGQUIT:
#endif // _WIN32
    case SIGTERM:
    case SIGINT:
        stop_web_server();
        break;
    }
}

int main(int argc, char *argv[])
{
    writeLog(0, "SubConverter " VERSION " starting up..", LOG_LEVEL_INFO);
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
    {
        //std::cerr<<"WSAStartup failed.\n";
        writeLog(0, "WSAStartup failed.", LOG_LEVEL_FATAL);
        return 1;
    }
    UINT origcp = GetConsoleOutputCP();
    SetConsoleOutputCP(65001);
#else
    signal(SIGPIPE, SIG_IGN);
    signal(SIGABRT, SIG_IGN);
    signal(SIGHUP, signal_handler);
    signal(SIGQUIT, signal_handler);
#endif // _WIN32
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    SetConsoleTitle("SubConverter " VERSION);
#ifndef _DEBUG
    std::string prgpath = argv[0];
    setcd(prgpath); //first switch to program directory
#endif // _DEBUG
    if(fileExist("pref.yml"))
        pref_path = "pref.yml";
    chkArg(argc, argv);
    setcd(pref_path); //then switch to pref directory
    readConf();
    if(!update_ruleset_on_request)
        refreshRulesets(rulesets, ruleset_content_array);
    generateBase();

    if(generator_mode)
    {
        int retVal = simpleGenerator();
#ifdef _WIN32
        SetConsoleOutputCP(origcp);
#endif // _WIN32
        return retVal;
    }

    append_response("GET", "/", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return "subconverter " VERSION " backend\n";
    });

    append_response("GET", "/version", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return "subconverter " VERSION " backend\n";
    });

    append_response("GET", "/refreshrules", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(access_token.size())
        {
            std::string token = getUrlArg(argument, "token");
            if(token != access_token)
            {
                *status_code = 403;
                return "Forbidden\n";
            }
        }
        refreshRulesets(rulesets, ruleset_content_array);
        generateBase();
        return "done\n";
    });

    append_response("GET", "/readconf", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(access_token.size())
        {
            std::string token = getUrlArg(argument, "token");
            if(token != access_token)
            {
                *status_code = 403;
                return "Forbidden\n";
            }
        }
        readConf();
        generateBase();
        return "done\n";
    });

    append_response("POST", "/updateconf", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(access_token.size())
        {
            std::string token = getUrlArg(argument, "token");
            if(token != access_token)
            {
                *status_code = 403;
                return "Forbidden\n";
            }
        }
        std::string type = getUrlArg(argument, "type");
        if(type == "form")
            fileWrite(pref_path, getFormData(postdata), true);
        else if(type == "direct")
            fileWrite(pref_path, postdata, true);
        else
        {
            *status_code = 501;
            return "Not Implemented\n";
        }

        readConf();
        if(!update_ruleset_on_request)
            refreshRulesets(rulesets, ruleset_content_array);
        generateBase();
        return "done\n";
    });

    append_response("GET", "/sub", "text/plain;charset=utf-8", subconverter);

    append_response("GET", "/sub2clashr", "text/plain;charset=utf-8", simpleToClashR);

    append_response("GET", "/surge2clash", "text/plain;charset=utf-8", surgeConfToClash);

    append_response("GET", "/getruleset", "text/plain;charset=utf-8", getRuleset);

    append_response("GET", "/getprofile", "text/plain;charset=utf-8", getProfile);

    append_response("GET", "/qx-script", "text/plain;charset=utf-8", getScript);

    append_response("GET", "/qx-rewrite", "text/plain;charset=utf-8", getRewriteRemote);

    append_response("GET", "/clash", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=clash", postdata, status_code, extra_headers);
    });

    append_response("GET", "/clashr", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=clashr", postdata, status_code, extra_headers);
    });

    append_response("GET", "/surge", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=surge", postdata, status_code, extra_headers);
    });

    append_response("GET", "/surfboard", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=surfboard", postdata, status_code, extra_headers);
    });

    append_response("GET", "/mellow", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=mellow", postdata, status_code, extra_headers);
    });

    append_response("GET", "/ss", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=ss", postdata, status_code, extra_headers);
    });

    append_response("GET", "/sssub", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=sssub", postdata, status_code, extra_headers);
    });

    append_response("GET", "/ssr", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=ssr", postdata, status_code, extra_headers);
    });

    append_response("GET", "/v2ray", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=v2ray", postdata, status_code, extra_headers);
    });

    append_response("GET", "/quan", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=quan", postdata, status_code, extra_headers);
    });

    append_response("GET", "/quanx", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=quanx", postdata, status_code, extra_headers);
    });

    append_response("GET", "/loon", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=loon", postdata, status_code, extra_headers);
    });

    append_response("GET", "/ssd", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=ssd", postdata, status_code, extra_headers);
    });

    append_response("GET", "/trojan", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=trojan", postdata, status_code, extra_headers);
    });

    if(!api_mode)
    {
        append_response("GET", "/get", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
        {
            std::string url = UrlDecode(getUrlArg(argument, "url"));
            return webGet(url, "");
        });

        append_response("GET", "/getlocal", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
        {
            return fileGet(UrlDecode(getUrlArg(argument, "path")));
        });
    }

    std::string env_port = GetEnv("PORT");
    if(env_port.size())
        listen_port = to_int(env_port, listen_port);
    listener_args args = {listen_address, listen_port, max_pending_connections, max_concurrent_threads};
    //std::cout<<"Serving HTTP @ http://"<<listen_address<<":"<<listen_port<<std::endl;
    writeLog(0, "Startup completed. Serving HTTP @ http://" + listen_address + ":" + std::to_string(listen_port), LOG_LEVEL_INFO);
    start_web_server_multi(&args);

#ifdef _WIN32
    WSACleanup();
#endif // _WIN32
    return 0;
}
