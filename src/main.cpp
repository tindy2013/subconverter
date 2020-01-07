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
#include "multithread.h"
#include "version.h"

//common settings
std::string pref_path = "pref.ini";
bool generator_mode = false;
string_array def_exclude_remarks, def_include_remarks, rulesets;
std::vector<ruleset_content> ruleset_content_array;
std::string listen_address = "127.0.0.1", default_url, managed_config_prefix;
int listen_port = 25500, max_pending_connections = 10, max_concurrent_threads = 4;
bool api_mode = true, write_managed_config = false, update_ruleset_on_request = false, overwrite_original_rules = true;
bool print_debug_info = false, cfw_child_process = false;
std::string access_token;
extern std::string custom_group;

//multi-thread lock
extern std::mutex on_configuring;

//preferences
string_array renames, emojis;
bool add_emoji = false, remove_old_emoji = false, append_proxy_type = false, filter_deprecated = true;
bool udp_flag = false, tfo_flag = false, scv_flag = false, do_sort = false;
std::string proxy_ruleset, proxy_subscription;

std::string clash_rule_base;
string_array clash_extra_group;
std::string surge_rule_base, surfboard_rule_base, mellow_rule_base;
std::string surge_ssr_path;

//pre-compiled rule bases
YAML::Node clash_base;
INIReader surge_base, mellow_base;

#ifndef _WIN32
void SetConsoleTitle(std::string title)
{
    system(std::string("echo \"\\033]0;" + title + "\\007\\c\"").data());
}
#endif // _WIN32

void setcd(char *argv[])
{
    std::string path;
    char szTmp[1024];
#ifndef _WIN32
    path.assign(argv[0]);
    if(path[0] != '/')
    {
        getcwd(szTmp, 1023);
        path.assign(szTmp);
        path.append("/");
        path.append(argv[0]);
    }
    path = path.substr(0, path.rfind("/") + 1);
#else
    GetModuleFileName(NULL, szTmp, 1023);
    strrchr(szTmp, '\\')[1] = '\0';
    path.assign(szTmp);
#endif // _WIN32
    chdir(path.data());
}

void refreshRulesets(string_array &ruleset_list, std::vector<ruleset_content> &rca)
{
    eraseElements(rca);
    std::string rule_group, rule_url;
    ruleset_content rc;

    std::string proxy;
    if(proxy_ruleset == "SYSTEM")
        proxy = getSystemProxy();
    else if(proxy_ruleset == "NONE")
        proxy = "";
    else
        proxy = proxy_ruleset;

    for(std::string &x : ruleset_list)
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
            rc = {rule_group, "", rule_url};
            rca.emplace_back(rc);
            continue;
        }
        else
        {
            std::cerr<<"Updating ruleset url '"<<rule_url<<"' with group '"<<rule_group<<"'."<<std::endl;
            if(fileExist(rule_url))
            {
                rc = {rule_group, rule_url, fileGet(rule_url, false)};
            }
            else if(rule_url.find("http://") == 0 || rule_url.find("https://") == 0)
            {
                rc = {rule_group, rule_url, webGet(rule_url, proxy)};
            }
            else
                continue;
        }
        if(rc.rule_content.size())
            rca.emplace_back(rc);
        else
            std::cerr<<"Warning: No data was fetched from this link. Skipping..."<<std::endl;
    }
}

void readConf()
{
    guarded_mutex guard(on_configuring);
    std::cerr<<"Reading preference settings..."<<std::endl;
    INIReader ini;
    ini.allow_dup_section_titles = true;
    //ini.do_utf8_to_gbk = true;
    int retVal = ini.ParseFile(pref_path);
    if(retVal != INIREADER_EXCEPTION_NONE)
    {
        std::cerr<<"Unable to load preference settings. Reason: "<<ini.GetLastError()<<"\n";
        return;
    }

    eraseElements(def_exclude_remarks);
    eraseElements(def_include_remarks);
    eraseElements(clash_extra_group);
    eraseElements(rulesets);
    string_array emojis_temp, renames_temp;

    ini.EnterSection("common");
    if(ini.ItemExist("api_mode"))
        api_mode = ini.GetBool("api_mode");
    if(ini.ItemExist("api_access_token"))
        access_token = ini.Get("api_access_token");
    if(ini.ItemExist("default_url"))
        default_url = ini.Get("default_url");
    if(ini.ItemPrefixExist("exclude_remarks"))
        ini.GetAll("exclude_remarks", def_exclude_remarks);
    if(ini.ItemPrefixExist("include_remarks"))
        ini.GetAll("include_remarks", def_include_remarks);
    if(ini.ItemExist("clash_rule_base"))
        clash_rule_base = ini.Get("clash_rule_base");
    if(ini.ItemExist("surge_rule_base"))
        surge_rule_base = ini.Get("surge_rule_base");
    if(ini.ItemExist("surfboard_rule_base"))
        surfboard_rule_base = ini.Get("surfboard_rule_base");
    if(ini.ItemExist("mellow_rule_base"))
        mellow_rule_base = ini.Get("mellow_rule_base");
    if(ini.ItemExist("append_proxy_type"))
        append_proxy_type = ini.GetBool("append_proxy_type");
    if(ini.ItemExist("proxy_ruleset"))
        proxy_ruleset = ini.Get("proxy_ruleset");
    if(ini.ItemExist("proxy_subscription"))
        proxy_subscription = ini.Get("proxy_subscription");
    if(ini.ItemPrefixExist("rename_node"))
    {
        ini.GetAll("rename_node", renames_temp);
        safe_set_renames(renames_temp);
    }

    if(ini.SectionExist("surge_external_proxy"))
    {
        ini.EnterSection("surge_external_proxy");
        if(ini.ItemExist("surge_ssr_path"))
            surge_ssr_path = ini.Get("surge_ssr_path");
    }

    if(ini.SectionExist("node_pref"))
    {
        ini.EnterSection("node_pref");
        if(ini.ItemExist("udp_flag"))
            udp_flag = ini.GetBool("udp_flag");
        if(ini.ItemExist("tcp_fast_open_flag"))
            tfo_flag = ini.GetBool("tcp_fast_open_flag");
        if(ini.ItemExist("sort_flag"))
            do_sort = ini.GetBool("sort_flag");
        if(ini.ItemExist("skip_cert_verify_flag"))
            scv_flag = ini.GetBool("skip_cert_verify_flag");
        if(ini.ItemExist("filter_deprecated_nodes"))
            filter_deprecated = ini.GetBool("filter_deprecated_nodes");
    }

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
    {
        ini.GetAll("rule", emojis_temp);
        safe_set_emojis(emojis_temp);
    }

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
    if(ini.ItemExist("print_debug_info"))
        print_debug_info = ini.GetBool("print_debug_info");
    if(ini.ItemExist("max_pending_connections"))
        max_pending_connections = ini.GetInt("max_pending_connections");
    if(ini.ItemExist("max_concurrent_threads"))
        max_concurrent_threads = ini.GetInt("max_concurrent_threads");

    std::cerr<<"Read preference settings completed."<<std::endl;
}

struct ExternalConfig
{
    string_array custom_proxy_group;
    string_array surge_ruleset;
    std::string clash_rule_base;
    std::string surge_rule_base;
    std::string surfboard_rule_base;
    std::string mellow_rule_base;
    bool overwrite_original_rules = true;
    bool enable_rule_generator = true;
};

int loadExternalConfig(std::string &path, ExternalConfig &ext, std::string proxy)
{
    std::string base_content;
    if(fileExist(path))
        base_content = fileGet(path, false);
    else
        base_content = webGet(path, proxy);

    INIReader ini;
    ini.store_isolated_line = true;
    ini.SetIsolatedItemsSection("custom");
    if(ini.Parse(base_content) != INIREADER_EXCEPTION_NONE)
    {
        std::cerr<<"Load external configuration failed. Reason: "<<ini.GetLastError()<<"\n";
        return -1;
    }

    ini.EnterSection("custom");
    if(ini.ItemPrefixExist("custom_proxy_group"))
        ini.GetAll("custom_proxy_group", ext.custom_proxy_group);
    if(ini.ItemPrefixExist("surge_ruleset"))
        ini.GetAll("surge_ruleset", ext.surge_ruleset);

    if(ini.ItemExist("clash_rule_base"))
        ext.clash_rule_base = ini.Get("clash_rule_base");
    if(ini.ItemExist("surge_rule_base"))
        ext.surge_rule_base = ini.Get("surge_rule_base");
    if(ini.ItemExist("surfboard_rule_base"))
        ext.surfboard_rule_base = ini.Get("surfboard_rule_base");
    if(ini.ItemExist("mellow_rule_base"))
        ext.mellow_rule_base = ini.Get("mellow_rule_base");

    if(ini.ItemExist("overwrite_original_rules"))
        ext.overwrite_original_rules = ini.GetBool("overwrite_original_rules");
    if(ini.ItemExist("enable_rule_generator"))
        ext.enable_rule_generator = ini.GetBool("enable_rule_generator");

    return 0;
}

void generateBase()
{
    std::string base_content;
    int retVal = 0;
    std::cerr<<"Generating base content for Clash/R...\n";
    if(fileExist(clash_rule_base))
        base_content = fileGet(clash_rule_base, false);
    else
        base_content = webGet(clash_rule_base, getSystemProxy());
    try
    {
        clash_base = YAML::Load(base_content);
        rulesetToClash(clash_base, ruleset_content_array);
    }
    catch (YAML::Exception &e)
    {
        std::cerr<<"Unable to load Clash base content. Reason: "<<e.msg<<"\n";
    }
    std::cerr<<"Generating base content for Mellow...\n";
    if(fileExist(mellow_rule_base))
        base_content = fileGet(mellow_rule_base, false);
    else
        base_content = webGet(mellow_rule_base, getSystemProxy());
    retVal = mellow_base.Parse(base_content);
    if(retVal != INIREADER_EXCEPTION_NONE)
        std::cerr<<"Unable to load Mellow base content. Reason: "<<mellow_base.GetLastError()<<"\n";
    else
        rulesetToSurge(mellow_base, ruleset_content_array, 2);
}

std::string subconverter(RESPONSE_CALLBACK_ARGS)
{
    std::string target = getUrlArg(argument, "target"), url = UrlDecode(getUrlArg(argument, "url")), emoji = getUrlArg(argument, "emoji");
    std::string group = UrlDecode(getUrlArg(argument, "group")), upload = getUrlArg(argument, "upload"), upload_path = getUrlArg(argument, "upload_path"), version = getUrlArg(argument, "ver");
    std::string append_type = getUrlArg(argument, "append_type"), tfo = getUrlArg(argument, "tfo"), udp = getUrlArg(argument, "udp"), nodelist = getUrlArg(argument, "list");
    std::string include = UrlDecode(getUrlArg(argument, "include")), exclude = UrlDecode(getUrlArg(argument, "exclude")), sort_flag = getUrlArg(argument, "sort");
    std::string scv = getUrlArg(argument, "scv"), fdn = getUrlArg(argument, "fdn");
    std::string base_content, output_content;
    string_array extra_group, extra_ruleset, include_remarks, exclude_remarks;
    std::string groups = urlsafe_base64_decode(getUrlArg(argument, "groups")), ruleset = urlsafe_base64_decode(getUrlArg(argument, "ruleset")), config = UrlDecode(getUrlArg(argument, "config"));
    std::vector<ruleset_content> rca;
    extra_settings ext;

    //for external configuration
    std::string ext_clash_base = clash_rule_base, ext_surge_base = surge_rule_base, ext_mellow_base = mellow_rule_base, ext_surfboard_base = surfboard_rule_base;

    //validate urls
    if(!url.size())
        url = default_url;
    if(!url.size() || !target.size())
        return "Invalid request!";

    //check if we need to read configuration
    if(!api_mode || cfw_child_process)
        readConf();

    //check for proxy settings
    std::string proxy;
    if(proxy_subscription == "SYSTEM")
        proxy = getSystemProxy();
    else if(proxy_subscription == "NONE")
        proxy = "";
    else
        proxy = proxy_subscription;

    //load external configuration
    if(config.size())
    {
        std::cerr<<"External configuration file provided. Loading...\n";
        //read predefined data first
        extra_group = clash_extra_group;
        extra_ruleset = rulesets;
        //then load external configuration
        ExternalConfig extconf;
        loadExternalConfig(config, extconf, proxy);
        if(extconf.clash_rule_base.size())
            ext_clash_base = extconf.clash_rule_base;
        if(extconf.surge_rule_base.size())
            ext_surge_base = extconf.surge_rule_base;
        if(extconf.surfboard_rule_base.size())
            ext_surfboard_base = extconf.surfboard_rule_base;
        if(extconf.mellow_rule_base.size())
            ext_mellow_base = extconf.mellow_rule_base;
        ext.enable_rule_generator = extconf.enable_rule_generator;
        //load custom group
        if(extconf.custom_proxy_group.size())
            extra_group = extconf.custom_proxy_group;
        //load custom rules
        if(extconf.overwrite_original_rules)
        {
            extra_ruleset = extconf.surge_ruleset;
            refreshRulesets(extra_ruleset, rca);
        }
    }
    else
    {
        //loading custom groups
        if(groups.size())
        {
            extra_group = split(groups, "@");
            if(!extra_group.size())
                extra_group = clash_extra_group;
        }
        else
            extra_group = clash_extra_group;

        //loading custom rulesets
        if(ruleset.size())
        {
            extra_ruleset = split(ruleset, "@");
            if(!extra_ruleset.size())
            {
                if(update_ruleset_on_request || cfw_child_process)
                    refreshRulesets(rulesets, ruleset_content_array);
                rca = ruleset_content_array;
            }
            else
            {
                refreshRulesets(extra_ruleset, rca);
            }
        }
        else
        {
            if(update_ruleset_on_request || cfw_child_process)
                refreshRulesets(rulesets, ruleset_content_array);
            rca = ruleset_content_array;
        }
    }

    //check other flags
    if(emoji.size())
    {
        ext.add_emoji = ext.remove_emoji = emoji == "true";
    }
    else
    {
        ext.add_emoji = add_emoji;
        ext.remove_emoji = remove_old_emoji;
    }
    if(append_type.size())
        ext.append_proxy_type = append_type == "true";
    else
        ext.append_proxy_type = append_proxy_type;

    ext.tfo = tfo.size() ? tfo == "true" : tfo_flag;
    ext.udp = udp.size() ? udp == "true" : udp_flag;
    ext.sort_flag = sort_flag.size() ? sort_flag == "true" : do_sort;
    ext.skip_cert_verify = scv.size() ? scv == "true" : scv_flag;
    ext.filter_deprecated = fdn.size() ? fdn == "true" : filter_deprecated;

    ext.nodelist = nodelist == "true";
    ext.surge_ssr_path = surge_ssr_path;

    //loading urls
    string_array urls = split(url, "|");
    std::vector<nodeInfo> nodes;
    int groupID = 0;

    //check custom include/exclude settings
    if(include.size() && regValid(include))
        include_remarks.emplace_back(include);
    else
        include_remarks = def_include_remarks;
    if(exclude.size() && regValid(exclude))
        exclude_remarks.emplace_back(exclude);
    else
        exclude_remarks = def_exclude_remarks;

    //check custom group name
    if(group.size())
        custom_group = group;

    //start parsing urls
    for(std::string &x : urls)
    {
        x = trim(x);
        std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        addNodes(x, nodes, groupID, proxy, exclude_remarks, include_remarks);
        groupID++;
    }
    //exit if found nothing
    if(!nodes.size())
        return "No nodes were found!";

    std::cerr<<"Generate target: ";
    if(target == "clash" || target == "clashr")
    {
        std::cerr<<"Clash"<<((target == "clashr") ? "R" : "")<<std::endl;
        if(rca.size() || extra_group.size() || update_ruleset_on_request || ext_clash_base != clash_rule_base)
        {
            if(fileExist(ext_clash_base))
                base_content = fileGet(ext_clash_base, false);
            else
                base_content = webGet(ext_clash_base, getSystemProxy());

            output_content = netchToClash(nodes, base_content, rca, extra_group, target == "clashr", ext);
        }
        else
        {
            YAML::Node yamlnode = clash_base;
            netchToClash(nodes, yamlnode, extra_group, target == "clashr", ext);
            output_content = YAML::Dump(yamlnode);
        }

        if(upload == "true")
            uploadGist(target, upload_path, output_content, false);
        return output_content;
    }
    else if(target == "surge")
    {
        int surge_ver = version.size() ? to_int(version, 3) : 3;
        std::cerr<<"Surge "<<surge_ver<<std::endl;

        if(fileExist(ext_surge_base))
            base_content = fileGet(ext_surge_base, false);
        else
            base_content = webGet(ext_surge_base, getSystemProxy());

        output_content = netchToSurge(nodes, base_content, rca, extra_group, surge_ver, ext);
        if(upload == "true")
            uploadGist("surge" + version, upload_path, output_content, true);

        if(write_managed_config && managed_config_prefix.size() && !ext.nodelist)
            output_content = "#!MANAGED-CONFIG " + managed_config_prefix + "/sub?" + argument + "\n\n" + output_content;
        return output_content;
    }
    else if(target == "surfboard")
    {
        std::cerr<<"Surfboard"<<std::endl;
        if(fileExist(ext_surfboard_base))
            base_content = fileGet(ext_surfboard_base, false);
        else
            base_content = webGet(ext_surfboard_base, getSystemProxy());

        output_content = netchToSurge(nodes, base_content, rca, extra_group, 2, ext);
        if(upload == "true")
            uploadGist("surfboard", upload_path, output_content, true);

        if(write_managed_config && managed_config_prefix.size())
            output_content = "#!MANAGED-CONFIG " + managed_config_prefix + "/sub?" + argument + "\n\n" + output_content;
        return output_content;
    }
    else if(target == "mellow")
    {
        std::cerr<<"Mellow"<<std::endl;
        if(rca.size() || extra_group.size() || update_ruleset_on_request || ext_mellow_base != mellow_rule_base)
        {
            if(fileExist(ext_mellow_base))
                base_content = fileGet(ext_mellow_base, false);
            else
                base_content = webGet(ext_mellow_base, getSystemProxy());

            output_content = netchToMellow(nodes, base_content, rca, extra_group, ext);
        }
        else
        {
            INIReader ini;
            ini = mellow_base;
            netchToMellow(nodes, ini, rca, extra_group, ext);
            output_content = ini.ToString();
        }

        if(upload == "true")
            uploadGist("mellow", upload_path, output_content, true);

        return output_content;
    }
    else if(target == "ss")
    {
        std::cerr<<"SS"<<std::endl;
        output_content = netchToSS(nodes, ext);
        if(upload == "true")
            uploadGist("ss", upload_path, output_content, false);
        return output_content;
    }
    else if(target == "ssr")
    {
        std::cerr<<"SSR"<<std::endl;
        output_content = netchToSSR(nodes, ext);
        if(upload == "true")
            uploadGist("ssr", upload_path, output_content, false);
        return output_content;
    }
    else if(target == "v2ray")
    {
        std::cerr<<"v2rayN"<<std::endl;
        output_content = netchToVMess(nodes, ext);
        if(upload == "true")
            uploadGist("v2ray", upload_path, output_content, false);
        return output_content;
    }
    else if(target == "quan")
    {
        std::cerr<<"Quantumult"<<std::endl;
        output_content = netchToQuan(nodes, ext);
        if(upload == "true")
            uploadGist("quan", upload_path, output_content, false);
        return output_content;
    }
    else if(target == "quanx")
    {
        std::cerr<<"Quantumult X"<<std::endl;
        output_content = netchToQuanX(nodes, ext);
        if(upload == "true")
            uploadGist("quanx", upload_path, output_content, false);
        return output_content;
    }
    else if(target == "ssd")
    {
        std::cerr<<"SSD"<<std::endl;
        output_content = netchToSSD(nodes, group, ext);
        if(upload == "true")
            uploadGist("ssd", upload_path, output_content, false);
        return output_content;
    }
    else
    {
        std::cerr<<"Unspecified"<<std::endl;
        return std::string();
    }
}

std::string simpleToClashR(RESPONSE_CALLBACK_ARGS)
{
    std::string url = argument.size() <= 8 ? "" : argument.substr(8);
    std::string base_content, output_content;
    std::vector<nodeInfo> nodes;
    string_array extra_group, extra_ruleset, include_remarks, exclude_remarks;
    std::vector<ruleset_content> rca;

    if(!url.size())
        url = default_url;
    if(!url.size() || argument.substr(0, 8) != "sublink=")
        return "Invalid request!";
    if(url == "sublink")
        return "Please insert your subscription link instead of clicking the default link.";
    if(!api_mode || cfw_child_process)
        readConf();

    extra_group = clash_extra_group;

    if(update_ruleset_on_request || cfw_child_process)
        refreshRulesets(rulesets, ruleset_content_array);
    rca = ruleset_content_array;

    extra_settings ext = {true, add_emoji, remove_old_emoji, append_proxy_type, udp_flag, tfo_flag, false, do_sort, scv_flag, filter_deprecated, ""};

    std::string proxy;
    if(proxy_subscription == "SYSTEM")
        proxy = getSystemProxy();
    else if(proxy_subscription == "NONE")
        proxy = "";
    else
        proxy = proxy_subscription;

    include_remarks = def_include_remarks;
    exclude_remarks = def_exclude_remarks;

    std::cerr<<"Fetching node data from url '"<<url<<"'.\n";
    addNodes(url, nodes, 0, proxy, exclude_remarks, include_remarks);

    if(!nodes.size())
        return "No nodes were found!";

    std::cerr<<"Generate target: ClashR\n";

    YAML::Node yamlnode = clash_base;
    netchToClash(nodes, yamlnode, extra_group, true, ext);
    return YAML::Dump(yamlnode);
}

void chkArg(int argc, char *argv[])
{
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-cfw") == 0)
            cfw_child_process = true;
        else if(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0)
            pref_path.assign(argv[++i]);
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

#ifndef _DEBUG
    setcd(argv);
#endif // _DEBUG
    SetConsoleTitle("subconverter " VERSION);
    chkArg(argc, argv);
    readConf();
    if(!update_ruleset_on_request)
        refreshRulesets(rulesets, ruleset_content_array);
    generateBase();

    append_response("GET", "/", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return std::string("subconverter " VERSION " backend");
    });

    append_response("GET", "/refreshrules", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(access_token.size())
        {
            std::string token = getUrlArg(argument, "token");
            if(token != access_token)
                return "Unauthorized";
        }
        refreshRulesets(rulesets, ruleset_content_array);
        generateBase();
        return "done";
    });

    append_response("GET", "/readconf", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(access_token.size())
        {
            std::string token = getUrlArg(argument, "token");
            if(token != access_token)
                return "Unauthorized";
        }
        readConf();
        generateBase();
        return "done";
    });

    append_response("POST", "/updateconf", "text/plain", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        if(access_token.size())
        {
            std::string token = getUrlArg(argument, "token");
            if(token != access_token)
                return "Unauthorized";
        }
        std::string type = getUrlArg(argument, "type");
        if(type == "form")
            fileWrite(pref_path, getFormData(postdata), true);
        else if(type == "direct")
            fileWrite(pref_path, postdata, true);
        else
            return "Not implemented";
        readConf();
        if(!update_ruleset_on_request)
            refreshRulesets(rulesets, ruleset_content_array);
        generateBase();
        return "done";
    });

    append_response("GET", "/sub", "text/plain;charset=utf-8", subconverter);

    append_response("GET", "/sub2clashr", "text/plain;charset=utf-8", simpleToClashR);

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

    append_response("GET", "/surfboard", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=surfboard", postdata);
    });

    append_response("GET", "/mellow", "text/plain;charset=utf-8", [](RESPONSE_CALLBACK_ARGS) -> std::string
    {
        return subconverter(argument + "&target=mellow", postdata);
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

    std::string env_port = GetEnv("PORT");
    if(env_port.size())
        listen_port = to_int(env_port, listen_port);
    listener_args args = {listen_address, listen_port, max_pending_connections, max_concurrent_threads};
    std::cout<<"Serving HTTP @ http://"<<listen_address<<":"<<listen_port<<std::endl;
    start_web_server_multi(&args);

#ifdef _WIN32
    WSACleanup();
#endif // _WIN32
    return 0;
}
