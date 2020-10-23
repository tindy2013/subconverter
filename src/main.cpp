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

extern std::string gPrefPath, gAccessToken, gListenAddress, gGenerateProfiles, gManagedConfigPrefix;
extern bool gAPIMode, gGeneratorMode, gCFWChildProcess, gUpdateRulesetOnRequest;
extern int gListenPort, gMaxConcurThreads, gMaxPendingConns;
extern string_array gCustomRulesets;
extern std::vector<ruleset_content> gRulesetContent;

#ifndef _WIN32
void SetConsoleTitle(const std::string &title)
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
            gCFWChildProcess = true;
            gUpdateRulesetOnRequest = true;
        }
        else if(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0)
        {
            if(i < argc - 1)
                gPrefPath.assign(argv[++i]);
        }
        else if(strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gen") == 0)
        {
            gGeneratorMode = true;
        }
        else if(strcmp(argv[i], "--artifact") == 0)
        {
            if(i < argc - 1)
                gGenerateProfiles.assign(argv[++i]);
        }
        else if(strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0)
        {
            if(i < argc - 1)
                if(freopen(argv[++i], "a", stderr) == NULL)
                    std::cerr<<"Error redirecting output to file.\n";
        }
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
#ifndef _DEBUG
    std::string prgpath = argv[0];
    setcd(prgpath); //first switch to program directory
#endif // _DEBUG
    if(fileExist("pref.yml"))
        gPrefPath = "pref.yml";
    chkArg(argc, argv);
    setcd(gPrefPath); //then switch to pref directory
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
    defer(SetConsoleOutputCP(origcp);)
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
    readConf();
    if(!gUpdateRulesetOnRequest)
        refreshRulesets(gCustomRulesets, gRulesetContent);

    std::string env_api_mode = GetEnv("API_MODE"), env_managed_prefix = GetEnv("MANAGED_PREFIX"), env_token = GetEnv("API_TOKEN");
    gAPIMode = tribool().parse(toLower(env_api_mode)).get(gAPIMode);
    if(env_managed_prefix.size())
        gManagedConfigPrefix = env_managed_prefix;
    if(env_token.size())
        gAccessToken = env_token;

    if(gGeneratorMode)
        return simpleGenerator();

    /*
    append_response("GET", "/", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return "subconverter " VERSION " backend\n";
    });
    */

    append_response("GET", "/version", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return "subconverter " VERSION " backend\n";
    });

    append_response("GET", "/refreshrules", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(gAccessToken.size())
        {
            std::string token = getUrlArg(request.argument, "token");
            if(token != gAccessToken)
            {
                response.status_code = 403;
                return "Forbidden\n";
            }
        }
        refreshRulesets(gCustomRulesets, gRulesetContent);
        return "done\n";
    });

    append_response("GET", "/readconf", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(gAccessToken.size())
        {
            std::string token = getUrlArg(request.argument, "token");
            if(token != gAccessToken)
            {
                response.status_code = 403;
                return "Forbidden\n";
            }
        }
        readConf();
        if(!gUpdateRulesetOnRequest)
            refreshRulesets(gCustomRulesets, gRulesetContent);
        return "done\n";
    });

    append_response("POST", "/updateconf", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(gAccessToken.size())
        {
            std::string token = getUrlArg(request.argument, "token");
            if(token != gAccessToken)
            {
                response.status_code = 403;
                return "Forbidden\n";
            }
        }
        std::string type = getUrlArg(request.argument, "type");
        if(type == "form")
            fileWrite(gPrefPath, getFormData(request.postdata), true);
        else if(type == "direct")
            fileWrite(gPrefPath, request.postdata, true);
        else
        {
            response.status_code = 501;
            return "Not Implemented\n";
        }

        readConf();
        if(!gUpdateRulesetOnRequest)
            refreshRulesets(gCustomRulesets, gRulesetContent);
        return "done\n";
    });

    append_response("GET", "/flushcache", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(getUrlArg(request.argument, "token") != gAccessToken)
        {
            response.status_code = 403;
            return "Forbidden";
        }
        flushCache();
        return "done";
    });

    append_response("GET", "/sub", "text/plain;charset=utf-8", subconverter);

    append_response("GET", "/sub2clashr", "text/plain;charset=utf-8", simpleToClashR);

    append_response("GET", "/surge2clash", "text/plain;charset=utf-8", surgeConfToClash);

    append_response("GET", "/getruleset", "text/plain;charset=utf-8", getRuleset);

    append_response("GET", "/getprofile", "text/plain;charset=utf-8", getProfile);

    append_response("GET", "/qx-script", "text/plain;charset=utf-8", getScript);

    append_response("GET", "/qx-rewrite", "text/plain;charset=utf-8", getRewriteRemote);

    append_response("GET", "/render", "text/plain;charset=utf-8", renderTemplate);

    append_response("GET", "/convert", "text/plain;charset=utf-8", getConvertedRuleset);

    if(!gAPIMode)
    {
        append_response("GET", "/get", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
        {
            std::string url = UrlDecode(getUrlArg(request.argument, "url"));
            return webGet(url, "");
        });

        append_response("GET", "/getlocal", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
        {
            return fileGet(UrlDecode(getUrlArg(request.argument, "path")));
        });
    }

    std::string env_port = GetEnv("PORT");
    if(env_port.size())
        gListenPort = to_int(env_port, gListenPort);
    listener_args args = {gListenAddress, gListenPort, gMaxPendingConns, gMaxConcurThreads};
    //std::cout<<"Serving HTTP @ http://"<<listen_address<<":"<<listen_port<<std::endl;
    writeLog(0, "Startup completed. Serving HTTP @ http://" + gListenAddress + ":" + std::to_string(gListenPort), LOG_LEVEL_INFO);
    start_web_server_multi(&args);

#ifdef _WIN32
    WSACleanup();
#endif // _WIN32
    return 0;
}
