#include <iostream>
#include <string>
#include <mutex>
#include <unistd.h>

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
std::string listen_address = "127.0.0.1", default_url, managed_config_prefix;
int listen_port = 25500, max_pending_connections = 10, max_concurrent_threads = 4;
bool api_mode = true, write_managed_config = true, update_ruleset_on_request = false, overwrite_original_rules = true;
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

void setcd(char *argv[])
{
#ifndef _WIN32
    std::string path, prgpath;
    char szTmp[1024];
    prgpath.assign(argv[0]);
    if(prgpath[0] == '/')
    {
        path = prgpath.substr(0, prgpath.rfind("/") + 1);
    }
    else
    {
        getcwd(szTmp, 1023);
        path.append(szTmp);
        path.append("/");
        path.append(argv[0]);
        path = path.substr(0, path.rfind("/") + 1);
    }
    chdir(path.data());
#endif // _WIN32
}

std::string refreshRulesets()
{
    guarded_mutex guard(on_configuring);
    eraseElements(ruleset_content_array);
    string_array vArray;
    std::string rule_group, rule_url, proxy = getSystemProxy();
    ruleset_content rc;
    for(std::string &x : rulesets)
    {
        /*
        vArray = split(x, ",");
        if(vArray.size() != 2)
            continue;
        rule_group = trim(vArray[0]);
        rule_url = trim(vArray[1]);
        */
        if(x.find(",") == x.npos)
            continue;
        rule_group = trim(x.substr(0, x.find(",")));
        rule_url = trim(x.substr(x.find(",") + 1));
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
        else
            std::cerr<<"Warning: No data was fetched from this link. Skipping..."<<std::endl;
    }

    return "done";
}

void readConf()
{
    guarded_mutex guard(on_configuring);
    std::cerr<<"Reading preference settings..."<<std::endl;
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

    ini.EnterSection("managed_config");
    if(ini.ItemExist("write_managed_config"))
        write_managed_config = ini.GetBool("write_managed_config");
    if(ini.ItemExist("managed_config_prefix"))
        managed_config_prefix = ini.Get("managed_config_prefix");

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
    std::cerr<<"Read preference settings completed."<<std::endl;
}

std::string subconverter(RESPONSE_CALLBACK_ARGS)
{
    if(!api_mode)
        readConf();
    std::string target = getUrlArg(argument, "target"), url = UrlDecode(getUrlArg(argument, "url")), include = UrlDecode(getUrlArg(argument, "regex"));
    std::string group = UrlDecode(getUrlArg(argument, "group")), upload = getUrlArg(argument, "upload"), version = getUrlArg(argument, "ver");
    std::string base_content, output_content;
    if(!url.size())
        url = default_url;
    string_array urls = split(url, "|");
    std::vector<nodeInfo> nodes;
    int groupID = 0;
    if(include.size())
    {
        eraseElements(def_include_remarks);
        def_include_remarks.emplace_back(include);
    }
    if(group.size())
        custom_group = group;
    for(std::string &x : urls)
    {
        std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        addNodes(x, nodes, groupID);
        groupID++;
    }
    if(!target.size())
        return std::string();

    std::cerr<<"Generate target: ";
    if(target == "clash" || target == "clashr")
    {
        std::cerr<<"Clash"<<((target == "clashr") ? "R" : "")<<std::endl;
        if(fileExist(clash_rule_base))
            base_content = fileGet(clash_rule_base, false);
        else
            base_content = webGet(clash_rule_base, getSystemProxy());

        if(update_ruleset_on_request)
            refreshRulesets();
        output_content = netchToClash(nodes, base_content, ruleset_content_array, clash_extra_group, target == "clashr");
        if(upload == "true")
            uploadGist("clash", output_content, false);
        return output_content;
    }
    else if(target == "surge")
    {
        std::cerr<<"Surge "<<version<<std::endl;
        int surge_ver = version.size() ? stoi(version) : 3;
        if(fileExist(surge_rule_base))
            base_content = fileGet(surge_rule_base, false);
        else
            base_content = webGet(surge_rule_base, getSystemProxy());

        output_content = netchToSurge(nodes, base_content, rulesets, clash_extra_group, surge_ver);
        if(upload == "true")
            uploadGist("surge" + version, output_content, true);

        if(write_managed_config && managed_config_prefix.size())
            output_content = "#!MANAGED-CONFIG " + managed_config_prefix + "/sub?" + argument + "\n\n" + output_content;
        return output_content;
    }
    else if(target == "ss")
    {
        std::cerr<<"SS"<<std::endl;
        output_content = netchToSS(nodes);
        if(upload == "true")
            uploadGist("ss", output_content, false);
        return output_content;
    }
    else if(target == "ssr")
    {
        std::cerr<<"SSR"<<std::endl;
        output_content = netchToSSR(nodes);
        if(upload == "true")
            uploadGist("ssr", output_content, false);
        return output_content;
    }
    else if(target == "v2ray")
    {
        std::cerr<<"v2rayN"<<std::endl;
        output_content = netchToVMess(nodes);
        if(upload == "true")
            uploadGist("v2ray", output_content, false);
        return output_content;
    }
    else if(target == "quan")
    {
        std::cerr<<"Quantumult"<<std::endl;
        output_content = netchToQuan(nodes);
        if(upload == "true")
            uploadGist("quan", output_content, false);
        return output_content;
    }
    else if(target == "quanx")
    {
        std::cerr<<"Quantumult X"<<std::endl;
        output_content = netchToQuanX(nodes);
        if(upload == "true")
            uploadGist("quanx", output_content, false);
        return output_content;
    }
    else if(target == "ssd")
    {
        std::cerr<<"SSD"<<std::endl;
        output_content = netchToSSD(nodes, group);
        if(upload == "true")
            uploadGist("ssd", output_content, false);
        return output_content;
    }
    else
    {
        std::cerr<<"Unspecified"<<std::endl;
        return std::string();
    }
}

int main(int argc, char *argv[])
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

    setcd(argv);
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

    append_response("GET", "/sub", "text/plain;charset=utf-8", subconverter);

    append_response("GET", "/clash", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=clash", postdata);
    });

    append_response("GET", "/clashr", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=clashr", postdata);
    });

    append_response("GET", "/surge", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=surge", postdata);
    });

    append_response("GET", "/ss", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=ss", postdata);
    });

    append_response("GET", "/ssr", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=ssr", postdata);
    });

    append_response("GET", "/v2ray", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=v2ray", postdata);
    });

    append_response("GET", "/quan", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=quan", postdata);
    });

    append_response("GET", "/quanx", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=quanx", postdata);
    });

    append_response("GET", "/ssd", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=ssd", postdata);
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
