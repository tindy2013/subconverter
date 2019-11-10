#include <iostream>
#include <string>
#include <mutex>

#include <yaml-cpp/yaml.h>

#include "socket.h"
#include "misc.h"
#include "nodeinfo.h"
#include "speedtestutil.h"
#include "nodemanip.h"
#include "ini_reader.h"
#include "webget.h"
#include "webserver.h"
#include "subexport.h"

//common settings
string_array def_exclude_remarks, def_include_remarks, rulesets;
std::vector<ruleset_content> ruleset_content_array;
std::string listen_address = "127.0.0.1", default_url;
int listen_port = 25500, max_pending_connections = 10, max_concurrent_threads = 4;
bool api_mode = true, update_ruleset_on_request = false, overwrite_original_rules = true;
extern std::string custom_group;

//safety lock for multi-thread
typedef std::lock_guard<std::mutex> guarded_mutex;
std::mutex on_configuring;

//preferences
string_array renames, emojis;
bool add_emoji = false, remove_old_emoji = false;

//clash custom
std::string clash_rule_base;
string_array clash_extra_group;

//surge custom
std::string surge_rule_base;

std::string refreshRulesets()
{
    guarded_mutex guard(on_configuring);
    eraseElements(ruleset_content_array);
    string_array vArray;
    std::string rule_group, rule_url, proxy = getSystemProxy();
    ruleset_content rc;
    for(std::string &x : rulesets)
    {
        vArray = split(x, ",");
        if(vArray.size() != 2)
            continue;
        rule_group = trim(vArray[0]);
        rule_url = trim(vArray[1]);
        if(rule_url.find("[]") == 0)
        {
            std::cerr<<"Adding rule '"<<rule_url.substr(2)<<","<<rule_group<<"'."<<std::endl;
            rc = {rule_group, rule_url};
            ruleset_content_array.emplace_back(rc);
            continue;
        }
        else
        {
            std::cerr<<"Updating ruleset url '"<<rule_url<<"' with group '"<<rule_group<<"'."<<std::endl;
            if(fileExist(rule_url))
            {
                rc = {rule_group, fileGet(rule_url, false)};
            }
            else
            {
                rc = {rule_group, webGet(rule_url, proxy)};
            }
        }
        if(rc.rule_content.size())
            ruleset_content_array.emplace_back(rc);
    }

    return "done";
}

void readConf()
{
    guarded_mutex guard(on_configuring);
    eraseElements(def_exclude_remarks);
    eraseElements(def_include_remarks);
    eraseElements(clash_extra_group);
    eraseElements(rulesets);
    eraseElements(renames);
    eraseElements(emojis);
    INIReader ini;
    //ini.do_utf8_to_gbk = true;
    ini.ParseFile("pref.ini");

    ini.EnterSection("common");
    if(ini.ItemExist("api_mode"))
        api_mode = ini.GetBool("api_mode");
    if(ini.ItemExist("default_url"))
        default_url = ini.Get("default_url");
    if(ini.ItemPrefixExist("exclude_remarks"))
        ini.GetAll("exclude_remarks", def_exclude_remarks);
    if(ini.ItemPrefixExist("include_remarks"))
        ini.GetAll("include_remarks", def_exclude_remarks);
    if(ini.ItemExist("clash_rule_base"))
        clash_rule_base = ini.Get("clash_rule_base");
    if(ini.ItemExist("surge_rule_base"))
        surge_rule_base = ini.Get("surge_rule_base");
    if(ini.ItemPrefixExist("rename_node"))
        ini.GetAll("rename_node", renames);

    ini.EnterSection("emojis");
    if(ini.ItemExist("add_emoji"))
        add_emoji = ini.GetBool("add_emoji");
    if(ini.ItemExist("remove_old_emoji"))
        remove_old_emoji = ini.GetBool("remove_old_emoji");
    if(ini.ItemPrefixExist("rule"))
        ini.GetAll("rule", emojis);

    ini.EnterSection("ruleset");
    if(ini.GetBool("enabled"))
    {
        if(ini.ItemExist("overwrite_original_rules"))
            overwrite_original_rules = ini.GetBool("overwrite_original_rules");
        if(ini.ItemExist("update_ruleset_on_request"))
            update_ruleset_on_request = ini.GetBool("update_ruleset_on_request");
        if(ini.ItemPrefixExist("surge_ruleset"))
            ini.GetAll("surge_ruleset", rulesets);
    }
    else
    {
        overwrite_original_rules = false;
        update_ruleset_on_request = false;
    }

    ini.EnterSection("clash_proxy_group");
    if(ini.ItemPrefixExist("custom_proxy_group"))
        ini.GetAll("custom_proxy_group", clash_extra_group);

    ini.EnterSection("server");
    if(ini.ItemExist("listen"))
        listen_address = ini.Get("listen");
    if(ini.ItemExist("port"))
        listen_port = ini.GetInt("port");

    ini.EnterSection("advanced");
    if(ini.ItemExist("max_pending_connections"))
        max_pending_connections = ini.GetInt("max_pending_connections");
    if(ini.ItemExist("max_concurrent_threads"))
        max_concurrent_threads = ini.GetInt("max_concurrent_threads");

    remarksInit(def_exclude_remarks, def_include_remarks);
}

int main()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
    {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }
    SetConsoleOutputCP(65001);
#endif // _WIN32

    readConf();
    if(!update_ruleset_on_request)
        refreshRulesets();

    append_response("GET", "/refreshrules", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return refreshRulesets();
    });

    append_response("GET", "/readconf", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        readConf();
        return "done";
    });

    append_response("GET", "/clash", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string clash_base_content;
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex")), group = UrlDecode(getUrlArg(argument, "group"));
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        if(group.size()) custom_group = group;
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: Clash"<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }
        if(fileExist(clash_rule_base))
            clash_base_content = fileGet(clash_rule_base, false);
        else
            clash_base_content = webGet(clash_rule_base, getSystemProxy());

        if(update_ruleset_on_request)
            refreshRulesets();
        return netchToClash(nodes, clash_base_content, ruleset_content_array, clash_extra_group, false);
    });

    append_response("GET", "/clashr", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string clash_base_content;
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex")), group = UrlDecode(getUrlArg(argument, "group"));
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        if(group.size()) custom_group = group;
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: ClashR"<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }
        if(fileExist(clash_rule_base))
            clash_base_content = fileGet(clash_rule_base, false);
        else
            clash_base_content = webGet(clash_rule_base, getSystemProxy());

        if(update_ruleset_on_request)
            refreshRulesets();
        return netchToClash(nodes, clash_base_content, ruleset_content_array, clash_extra_group, true);
    });

    append_response("GET", "/surge", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string surge_base_content;
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex")), group = UrlDecode(getUrlArg(argument, "group")), version = getUrlArg(argument, "ver");
        int surge_ver = version.size() ? stoi(version) : 3;
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        if(group.size()) custom_group = group;
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: Surge "<<surge_ver<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }
        if(fileExist(surge_rule_base))
            surge_base_content = fileGet(surge_rule_base, false);
        else
            surge_base_content = webGet(surge_rule_base, getSystemProxy());

        return netchToSurge(nodes, surge_base_content, rulesets, clash_extra_group, surge_ver);
    });

    append_response("GET", "/ss", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex"));
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: SIP002"<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }

        return netchToSS(nodes);
    });

    append_response("GET", "/ssr", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex"));
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: SSR"<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }

        return netchToSSR(nodes);
    });

    append_response("GET", "/v2rayn", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex"));
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: VMess"<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }

        return netchToVMess(nodes);
    });

    append_response("GET", "/v2rayq", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex"));
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: VMess"<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }

        return netchToQuan(nodes);
    });

    append_response("GET", "/quanx", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex"));
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: Quantumult X"<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }

        return netchToQuanX(nodes);
    });

    append_response("GET", "/ssd", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(!api_mode)
            readConf();
        std::string url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex")), group = UrlDecode(getUrlArg(argument, "group"));
        if(!url.size()) url = default_url;
        string_array urls = split(url, "|");
        std::vector<nodeInfo> nodes;
        int groupID = 0;
        if(include.size())
        {
            eraseElements(def_include_remarks);
            def_include_remarks.emplace_back(include);
        }
        for(std::string &x : urls)
        {
            std::cerr<<"Fetching node data from url '"<<x<<"'. Generate target: SSD"<<std::endl;
            addNodes(x, nodes, groupID);
            groupID++;
        }

        return netchToSSD(nodes, group);
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

    listener_args args = {listen_address, listen_port, max_pending_connections, max_concurrent_threads};
    std::cout<<"Serving HTTP @ http://"<<listen_address<<":"<<listen_port<<std::endl;
    start_web_server_multi(&args);

#ifdef _WIN32
    WSACleanup();
#endif // _WIN32
    return 0;
}
