#include <iostream>
#include <string>
#include <mutex>
#include <numeric>

#include <yaml-cpp/yaml.h>

#include "misc.h"
#include "nodeinfo.h"
#include "speedtestutil.h"
#include "nodemanip.h"
#include "ini_reader.h"
#include "webget.h"
#include "webserver.h"
#include "subexport.h"
#include "multithread.h"
#include "logger.h"

//common settings
std::string pref_path = "pref.ini";
bool generator_mode = false;
string_array def_exclude_remarks, def_include_remarks, rulesets, stream_rules, time_rules;
std::vector<ruleset_content> ruleset_content_array;
std::string listen_address = "127.0.0.1", default_url, managed_config_prefix;
int listen_port = 25500, max_pending_connections = 10, max_concurrent_threads = 4;
bool api_mode = true, write_managed_config = false, enable_rule_generator = true, update_ruleset_on_request = false, overwrite_original_rules = true;
bool print_debug_info = false, cfw_child_process = false;
std::string access_token;
extern std::string custom_group;

//multi-thread lock
std::mutex on_configuring;

//preferences
string_array renames, emojis;
bool add_emoji = false, remove_old_emoji = false, append_proxy_type = false, filter_deprecated = true;
bool udp_flag = false, tfo_flag = false, scv_flag = false, do_sort = false;
std::string proxy_ruleset, proxy_subscription;

std::string clash_rule_base;
string_array clash_extra_group;
std::string surge_rule_base, surfboard_rule_base, mellow_rule_base, quan_rule_base, quanx_rule_base;
std::string surge_ssr_path;

//pre-compiled rule bases
YAML::Node clash_base;
INIReader surge_base, mellow_base;

string_array regex_blacklist = {"(.*)*"};

template <typename T> void operator >> (const YAML::Node& node, T& i)
{
    if(node.IsDefined()) //fail-safe
        i = node.as<T>();
};

std::string getRuleset(RESPONSE_CALLBACK_ARGS)
{
    std::string url = UrlDecode(getUrlArg(argument, "url")), type = getUrlArg(argument, "type"), group = UrlDecode(getUrlArg(argument, "group"));
    std::string output_content;

    if(!url.size() || !type.size() || !group.size() || (type != "1" && type != "2"))
    {
        *status_code = 400;
        return "Invalid request!";
    }

    if(fileExist(url))
        output_content = fileGet(url, false, true);
    else
        output_content = webGet(url, "");

    if(!output_content.size())
    {
        *status_code = 400;
        return "Invalid request!";
    }

    if(type == "2")
    {
        std::string strLine;
        std::stringstream ss;
        const std::string rule_match_regex = "^(.*?,.*?)(,.*)(,.*)$";

        ss << output_content;
        char delimiter = count(output_content.begin(), output_content.end(), '\n') < 1 ? '\r' : '\n';
        std::string::size_type lineSize;

        output_content.clear();

        while(getline(ss, strLine, delimiter))
        {
            if(strLine.find("IP-CIDR6") == 0 || strLine.find("URL-REGEX") == 0 || strLine.find("PROCESS-NAME") == 0 || strLine.find("AND") == 0 || strLine.find("OR") == 0) //remove unsupported types
                continue;

            lineSize = strLine.size();

            if(!strLine.empty() && (strLine[0] != ';' && strLine[0] != '#' && !(lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')))
            {
                strLine += "," + group;

                if(std::count(strLine.begin(), strLine.end(), ',') > 2 && regReplace(strLine, rule_match_regex, "$2") == ",no-resolve")
                    strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                else
                    strLine = regReplace(strLine, rule_match_regex, "$1$3");
            }

            output_content.append(strLine + "\n");
        }
    }

    return output_content;
}

int importItems(string_array &target)
{
    string_array result;
    std::stringstream ss;
    std::string path, content, strLine;
    unsigned int itemCount = 0;
    for(std::string &x : target)
    {
        if(x.find("!!import:") == x.npos)
        {
            result.emplace_back(x);
            continue;
        }
        path = x.substr(x.find(":") + 1);
        writeLog(0, "Trying to import items from " + path);

        if(fileExist(path))
            content = fileGet(path, false, api_mode);
        else
            content = webGet(path, "");
        if(!content.size())
            return -1;

        ss << content;
        char delimiter = count(content.begin(), content.end(), '\n') < 1 ? '\r' : '\n';
        std::string::size_type lineSize;
        while(getline(ss, strLine, delimiter))
        {
            lineSize = strLine.size();
            if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
            {
                strLine = strLine.substr(0, lineSize - 1);
                lineSize--;
            }
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            result.emplace_back(strLine);
            itemCount++;
        }
        ss.clear();
    }
    target.swap(result);
    writeLog(0, "Imported " + std::to_string(itemCount) + " item(s).");
    return 0;
}

void readRegexMatch(YAML::Node node, std::string delimiter, string_array &dest)
{
    YAML::Node object;
    std::string url, match, rep, strLine;

    for(unsigned i = 0; i < node.size(); i++)
    {
        object = node[i];
        object["import"] >> url;
        if(url.size())
        {
            url = "!!import:" + url;
            dest.emplace_back(url);
            continue;
        }
        object["match"] >> match;
        object["replace"] >> rep;
        if(match.size() && rep.size())
            strLine = match + delimiter + rep;
        else
            continue;
        dest.emplace_back(strLine);
    }
    importItems(dest);
}

void readEmoji(YAML::Node node, string_array &dest)
{
    YAML::Node object;
    std::string url, match, rep, strLine;

    for(unsigned i = 0; i < node.size(); i++)
    {
        object = node[i];
        object["import"] >> url;
        if(url.size())
        {
            url = "!!import:" + url;
            dest.emplace_back(url);
            continue;
        }
        object["match"] >> match;
        object["emoji"] >> rep;
        if(match.size() && rep.size())
            strLine = match + "," + rep;
        else
            continue;
        dest.emplace_back(strLine);
    }
    importItems(dest);
}

void readGroup(YAML::Node node, string_array &dest)
{
    std::string strLine, name, type, url, interval;
    string_array tempArray;
    YAML::Node object;
    unsigned int i, j;

    for(i = 0; i < node.size(); i++)
    {
        eraseElements(tempArray);
        object = node[i];
        object["import"] >> name;
        if(name.size())
        {
            name = "!!import:" + name;
            dest.emplace_back(name);
            continue;
        }
        url = "http://www.gstatic.com/generate_204", interval = "300";
        object["name"] >> name;
        object["type"] >> type;
        tempArray.emplace_back(name);
        tempArray.emplace_back(type);
        object["url"] >> url;
        object["interval"] >> interval;
        for(j = 0; j < object["rule"].size(); j++)
            tempArray.emplace_back(object["rule"][j].as<std::string>());
        if(type != "select" && type != "ssid")
        {
            tempArray.emplace_back(url);
            tempArray.emplace_back(interval);
        }

        if((type == "select" && tempArray.size() < 3) || (type == "ssid" && tempArray.size() < 4) || (type != "select" && type != "ssid" && tempArray.size() < 5))
            continue;

        strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b) -> std::string
        {
            return std::move(a) + "`" + std::move(b);
        });
        dest.emplace_back(strLine);
    }
    importItems(dest);
}

void readRuleset(YAML::Node node, string_array &dest)
{
    std::string strLine, name, url, group;
    YAML::Node object;

    for(unsigned int i = 0; i < node.size(); i++)
    {
        name = "";
        object = node[i];
        object["import"] >> name;
        if(name.size())
        {
            name = "!!import:" + name;
            dest.emplace_back(name);
            continue;
        }
        object["ruleset"] >> url;
        object["group"] >> group;
        object["rule"] >> name;
        if(url.size())
            strLine = group + "," + url;
        else if(name.size())
            strLine = group + ",[]" + name;
        else
            continue;
        dest.emplace_back(strLine);
    }
    importItems(dest);
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

void readYAMLConf(YAML::Node &node)
{
    YAML::Node section = node["common"];
    std::string strLine;
    string_array tempArray;

    section["api_mode"] >> api_mode;
    section["api_access_token"] >> access_token;
    if(section["default_url"].IsSequence())
    {
        section["default_url"] >> tempArray;
        if(tempArray.size())
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
        {
            return std::move(a) + "|" + std::move(b);
        });
        default_url = strLine;
    }
    if(section["exclude_remarks"].IsSequence())
        section["exclude_remarks"] >> def_exclude_remarks;
    if(section["include_remarks"].IsSequence())
        section["include_remarks"] >> def_include_remarks;
    section["clash_rule_base"] >> clash_rule_base;
    section["surge_rule_base"] >> surge_rule_base;
    section["surfboard_rule_base"] >> surfboard_rule_base;
    section["mellow_rule_base"] >> mellow_rule_base;
    section["quan_rule_base"] >> quan_rule_base;
    section["quanx_rule_base"] >> quanx_rule_base;

    section["append_proxy_type"] >> append_proxy_type;
    section["proxy_ruleset"] >> proxy_ruleset;
    section["proxy_subscription"] >> proxy_subscription;

    if(node["userinfo"].IsDefined())
    {
        section = node["userinfo"];
        if(section["stream_rule"].IsSequence())
        {
            readRegexMatch(section["stream_rule"], "@", tempArray);
            safe_set_streams(tempArray);
            eraseElements(tempArray);
        }
        if(section["time_rule"].IsSequence())
        {
            readRegexMatch(section["time_rule"], "@", tempArray);
            safe_set_times(tempArray);
            eraseElements(tempArray);
        }
    }

    if(node["node_pref"].IsDefined())
    {
        section = node["node_pref"];
        section["udp_flag"] >> udp_flag;
        section["tcp_fast_open_flag"] >> tfo_flag;
        section["sort_flag"] >> do_sort;
        section["skip_cert_verify_flag"] >> scv_flag;
    }

    if(section["rename_node"].IsSequence())
    {
        readRegexMatch(section["rename_node"], "@", tempArray);
        safe_set_renames(tempArray);
        eraseElements(tempArray);
    }

    if(node["managed_config"].IsDefined())
    {
        section = node["managed_config"];
        section["write_managed_config"] >> write_managed_config;
        section["managed_config_prefix"] >> managed_config_prefix;
    }

    if(node["surge_external_proxy"].IsDefined())
        node["surge_external_proxy"]["surge_ssr_path"] >> surge_ssr_path;

    if(node["emojis"].IsDefined())
    {
        section = node["emojis"];
        section["add_emoji"] >> add_emoji;
        section["remove_old_emoji"] >> remove_old_emoji;
        if(section["rules"].IsSequence())
        {
            readEmoji(section["rules"], tempArray);
            safe_set_emojis(tempArray);
            eraseElements(tempArray);
        }
    }

    if(node["ruleset"].IsDefined())
    {
        section = node["ruleset"];
        section["enabled"] >> enable_rule_generator;
        if(!enable_rule_generator)
        {
            overwrite_original_rules = false;
            update_ruleset_on_request = false;
        }
        else
        {
            section["overwrite_original_rules"] >> overwrite_original_rules;
            section["update_ruleset_on_request"] >> update_ruleset_on_request;
        }
        if(section["surge_ruleset"].IsSequence())
            readRuleset(section["surge_ruleset"], rulesets);
    }

    if(node["proxy_group"].IsDefined() && node["proxy_group"]["custom_proxy_group"].IsDefined())
        readGroup(node["proxy_group"]["custom_proxy_group"], clash_extra_group);

    if(node["server"].IsDefined())
    {
        node["server"]["listen"] >> listen_address;
        node["server"]["port"] >> listen_port;
    }

    if(node["advanced"].IsDefined())
    {
        node["advanced"]["print_debug_info"] >> print_debug_info;
        node["advanced"]["max_pending_connections"] >> max_pending_connections;
        node["advanced"]["max_concurrent_threads"] >> max_concurrent_threads;
    }
}

void readConf()
{
    guarded_mutex guard(on_configuring);
    std::cerr<<"Reading preference settings..."<<std::endl;

    eraseElements(def_exclude_remarks);
    eraseElements(def_include_remarks);
    eraseElements(clash_extra_group);
    eraseElements(rulesets);

    try
    {
        YAML::Node yaml = YAML::LoadFile(pref_path);
        if(yaml.size() && yaml["common"])
            return readYAMLConf(yaml);
    }
    catch (YAML::Exception &e)
    {
        //ignore
    }

    INIReader ini;
    ini.allow_dup_section_titles = true;
    //ini.do_utf8_to_gbk = true;
    int retVal = ini.ParseFile(pref_path);
    if(retVal != INIREADER_EXCEPTION_NONE)
    {
        std::cerr<<"Unable to load preference settings. Reason: "<<ini.GetLastError()<<"\n";
        return;
    }

    string_array tempArray;

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
    if(ini.ItemExist("quan_rule_base"))
        quan_rule_base = ini.Get("quan_rule_base");
    if(ini.ItemExist("quanx_rule_base"))
        quanx_rule_base = ini.Get("quanx_rule_base");
    if(ini.ItemExist("append_proxy_type"))
        append_proxy_type = ini.GetBool("append_proxy_type");
    if(ini.ItemExist("proxy_ruleset"))
        proxy_ruleset = ini.Get("proxy_ruleset");
    if(ini.ItemExist("proxy_subscription"))
        proxy_subscription = ini.Get("proxy_subscription");

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
        if(ini.ItemPrefixExist("rename_node"))
        {
            ini.GetAll("rename_node", tempArray);
            importItems(tempArray);
            safe_set_renames(tempArray);
            eraseElements(tempArray);
        }
    }

    if(ini.SectionExist("userinfo"))
    {
        ini.EnterSection("userinfo");
        if(ini.ItemPrefixExist("stream_rule"))
        {
            ini.GetAll("stream_rule", tempArray);
            importItems(tempArray);
            safe_set_streams(tempArray);
            eraseElements(tempArray);
        }
        if(ini.ItemPrefixExist("time_rule"))
        {
            ini.GetAll("time_rule", tempArray);
            importItems(tempArray);
            safe_set_times(tempArray);
            eraseElements(tempArray);
        }
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
        ini.GetAll("rule", tempArray);
        importItems(tempArray);
        safe_set_emojis(tempArray);
        eraseElements(tempArray);
    }

    ini.EnterSection("ruleset");
    enable_rule_generator = ini.GetBool("enabled");
    if(enable_rule_generator)
    {
        if(ini.ItemExist("overwrite_original_rules"))
            overwrite_original_rules = ini.GetBool("overwrite_original_rules");
        if(ini.ItemExist("update_ruleset_on_request"))
            update_ruleset_on_request = ini.GetBool("update_ruleset_on_request");
        if(ini.ItemPrefixExist("surge_ruleset"))
        {
            ini.GetAll("surge_ruleset", rulesets);
            importItems(rulesets);
        }
    }
    else
    {
        overwrite_original_rules = false;
        update_ruleset_on_request = false;
    }

    ini.EnterSection("clash_proxy_group");
    if(ini.ItemPrefixExist("custom_proxy_group"))
    {
        ini.GetAll("custom_proxy_group", clash_extra_group);
        importItems(clash_extra_group);
    }

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
    std::string quan_rule_base;
    std::string quanx_rule_base;
    string_array rename;
    string_array emoji;
    bool overwrite_original_rules = false;
    bool enable_rule_generator = true;
};

int loadExternalYAML(YAML::Node &node, ExternalConfig &ext)
{
    YAML::Node section = node["custom"], object;
    std::string name, type, url, interval;
    std::string group, strLine;

    section["clash_rule_base"] >> ext.clash_rule_base;
    section["surge_rule_base"] >> ext.surge_rule_base;
    section["surfboard_rule_base"] >> ext.surfboard_rule_base;
    section["mellow_rule_base"] >> ext.mellow_rule_base;
    section["quan_rule_base"] >> ext.quan_rule_base;
    section["quanx_rule_base"] >> ext.quanx_rule_base;

    ext.enable_rule_generator = section["enable_rule_generator"].as<bool>();
    ext.overwrite_original_rules = section["overwrite_original_rules"].as<bool>();

    if(section["custom_proxy_group"].size())
        readGroup(section["custom_proxy_group"], ext.custom_proxy_group);

    if(section["surge_ruleset"].size())
        readRuleset(section["surge_ruleset"], ext.surge_ruleset);

    if(section["rename_node"].size())
        readRegexMatch(section["rename_node"], "@", ext.rename);

    return 0;
}

int loadExternalConfig(std::string &path, ExternalConfig &ext, std::string proxy)
{
    std::string base_content;
    if(fileExist(path))
        base_content = fileGet(path, false, api_mode);
    else
        base_content = webGet(path, proxy);

    try
    {
        YAML::Node yaml = YAML::Load(base_content);
        if(yaml.size() && yaml["custom"])
            return loadExternalYAML(yaml, ext);
    }
    catch (YAML::Exception &e)
    {
        //ignore
    }

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
    {
        ini.GetAll("custom_proxy_group", ext.custom_proxy_group);
        importItems(ext.custom_proxy_group);
    }
    if(ini.ItemPrefixExist("surge_ruleset"))
    {
        ini.GetAll("surge_ruleset", ext.surge_ruleset);
        importItems(ext.surge_ruleset);
    }

    if(ini.ItemExist("clash_rule_base"))
        ext.clash_rule_base = ini.Get("clash_rule_base");
    if(ini.ItemExist("surge_rule_base"))
        ext.surge_rule_base = ini.Get("surge_rule_base");
    if(ini.ItemExist("surfboard_rule_base"))
        ext.surfboard_rule_base = ini.Get("surfboard_rule_base");
    if(ini.ItemExist("mellow_rule_base"))
        ext.mellow_rule_base = ini.Get("mellow_rule_base");
    if(ini.ItemExist("quan_rule_base"))
        ext.quan_rule_base = ini.Get("quan_rule_base");
    if(ini.ItemExist("quanx_rule_base"))
        ext.quanx_rule_base = ini.Get("quanx_rule_base");

    if(ini.ItemExist("overwrite_original_rules"))
        ext.overwrite_original_rules = ini.GetBool("overwrite_original_rules");
    if(ini.ItemExist("enable_rule_generator"))
        ext.enable_rule_generator = ini.GetBool("enable_rule_generator");

    if(ini.ItemPrefixExist("rename"))
    {
        ini.GetAll("rename", ext.rename);
        importItems(ext.rename);
    }
    if(ini.ItemPrefixExist("emoji"))
    {
        ini.GetAll("emoji", ext.emoji);
        importItems(ext.emoji);
    }

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
        rulesetToClash(clash_base, ruleset_content_array, overwrite_original_rules);
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
    mellow_base.keep_empty_section = true;
    mellow_base.store_any_line = true;
    retVal = mellow_base.Parse(base_content);
    if(retVal != INIREADER_EXCEPTION_NONE)
        std::cerr<<"Unable to load Mellow base content. Reason: "<<mellow_base.GetLastError()<<"\n";
    else
        rulesetToSurge(mellow_base, ruleset_content_array, 0, overwrite_original_rules);
}

std::string subconverter(RESPONSE_CALLBACK_ARGS)
{
    std::string target = getUrlArg(argument, "target"), url = UrlDecode(getUrlArg(argument, "url")), emoji = getUrlArg(argument, "emoji");
    std::string group = UrlDecode(getUrlArg(argument, "group")), upload = getUrlArg(argument, "upload"), upload_path = getUrlArg(argument, "upload_path"), version = getUrlArg(argument, "ver");
    std::string append_type = getUrlArg(argument, "append_type"), tfo = getUrlArg(argument, "tfo"), udp = getUrlArg(argument, "udp"), nodelist = getUrlArg(argument, "list");
    std::string include = UrlDecode(getUrlArg(argument, "include")), exclude = UrlDecode(getUrlArg(argument, "exclude")), sort_flag = getUrlArg(argument, "sort");
    std::string scv = getUrlArg(argument, "scv"), fdn = getUrlArg(argument, "fdn"), token = getUrlArg(argument, "token");
    std::string base_content, output_content;
    string_array extra_group, extra_ruleset, include_remarks, exclude_remarks;
    std::string groups = urlsafe_base64_decode(getUrlArg(argument, "groups")), ruleset = urlsafe_base64_decode(getUrlArg(argument, "ruleset")), config = UrlDecode(getUrlArg(argument, "config"));
    std::vector<ruleset_content> rca;
    extra_settings ext;
    std::string subInfo;
    bool ruleset_updated = false;

    if(std::find(regex_blacklist.cbegin(), regex_blacklist.cend(), include) != regex_blacklist.cend() || std::find(regex_blacklist.cbegin(), regex_blacklist.cend(), exclude) != regex_blacklist.cend())
        return "Invalid request!";

    //for external configuration
    std::string ext_clash_base = clash_rule_base, ext_surge_base = surge_rule_base, ext_mellow_base = mellow_rule_base, ext_surfboard_base = surfboard_rule_base;
    std::string ext_quan_base = quan_rule_base, ext_quanx_base = quanx_rule_base;

    //validate urls
    if(!url.size() && (!api_mode || token == access_token))
        url = default_url;
    if(!url.size() || !target.size())
    {
        *status_code = 400;
        return "Invalid request!";
    }

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

    ext.emoji_array = safe_get_emojis();
    ext.rename_array = safe_get_renames();

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
        if(extconf.quan_rule_base.size())
            ext_quan_base = extconf.quan_rule_base;
        if(extconf.quanx_rule_base.size())
            ext_quanx_base = extconf.quanx_rule_base;
        if(extconf.rename.size())
            ext.rename_array = extconf.rename;
        if(extconf.emoji.size())
            ext.emoji_array = extconf.emoji;
        ext.enable_rule_generator = extconf.enable_rule_generator;
        //load custom group
        if(extconf.custom_proxy_group.size())
            extra_group = extconf.custom_proxy_group;
        //load custom rules
        ext.overwrite_original_rules = extconf.overwrite_original_rules;
        if(extconf.surge_ruleset.size())
        {
            extra_ruleset = extconf.surge_ruleset;
            refreshRulesets(extra_ruleset, rca);
            ruleset_updated = true;
        }
        else
        {
            if(ext.enable_rule_generator)
            {
                if(update_ruleset_on_request)
                    refreshRulesets(rulesets, ruleset_content_array);
                rca = ruleset_content_array;
            }
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
                if(update_ruleset_on_request)
                    refreshRulesets(rulesets, ruleset_content_array);
                rca = ruleset_content_array;
            }
            else
            {
                refreshRulesets(extra_ruleset, rca);
                ruleset_updated = true;
            }
        }
        else
        {
            if(enable_rule_generator)
            {
                if(update_ruleset_on_request || cfw_child_process)
                    refreshRulesets(rulesets, ruleset_content_array);
                rca = ruleset_content_array;
            }
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
    ext.enable_rule_generator = enable_rule_generator;

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

    //start parsing urls
    string_array stream_temp = safe_get_streams(), time_temp = safe_get_times();
    for(std::string &x : urls)
    {
        x = trim(x);
        std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        if(addNodes(x, nodes, groupID, proxy, exclude_remarks, include_remarks, stream_temp, time_temp, subInfo) == -1)
        {
            *status_code = 400;
            return std::string("The following link doesn't contain any valid node info: " + x);
        }
        groupID++;
    }
    //exit if found nothing
    if(!nodes.size())
    {
        *status_code = 400;
        return "No nodes were found!";
    }

    //check custom group name
    if(group.size())
        for(nodeInfo &x : nodes)
            x.group = group;

    if(subInfo.size() && groupID == 1)
        extra_headers.emplace("Subscription-UserInfo", subInfo);

    std::cerr<<"Generate target: ";
    if(target == "clash" || target == "clashr")
    {
        std::cerr<<"Clash"<<((target == "clashr") ? "R" : "")<<std::endl;
        if(ruleset_updated || update_ruleset_on_request || ext_clash_base != clash_rule_base)
        {
            if(fileExist(ext_clash_base))
                base_content = fileGet(ext_clash_base, false);
            else
                base_content = webGet(ext_clash_base, getSystemProxy());

            output_content = netchToClash(nodes, base_content, rca, extra_group, target == "clashr", ext);
        }
        else
        {
            YAML::Node yamlnode = safe_get_clash_base();
            netchToClash(nodes, yamlnode, extra_group, target == "clashr", ext);
            output_content = YAML::Dump(yamlnode);
        }

        if(upload == "true")
            uploadGist(target, upload_path, output_content, false);
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
    }
    else if(target == "mellow")
    {
        std::cerr<<"Mellow"<<std::endl;
        if(ruleset_updated || update_ruleset_on_request || ext_mellow_base != mellow_rule_base)
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
            ini = safe_get_mellow_base();
            netchToMellow(nodes, ini, rca, extra_group, ext);
            output_content = ini.ToString();
        }

        if(upload == "true")
            uploadGist("mellow", upload_path, output_content, true);
    }
    else if(target == "ss")
    {
        std::cerr<<"SS"<<std::endl;
        output_content = netchToSS(nodes, ext);
        if(upload == "true")
            uploadGist("ss", upload_path, output_content, false);
        return output_content;
    }
    else if(target == "sssub")
    {
        std::cerr<<"SS Subscription"<<std::endl;
        output_content = netchToSSSub(nodes, ext);
        if(upload == "true")
            uploadGist("sssub", upload_path, output_content, false);
    }
    else if(target == "ssr")
    {
        std::cerr<<"SSR"<<std::endl;
        output_content = netchToSSR(nodes, ext);
        if(upload == "true")
            uploadGist("ssr", upload_path, output_content, false);
    }
    else if(target == "v2ray")
    {
        std::cerr<<"v2rayN"<<std::endl;
        output_content = netchToVMess(nodes, ext);
        if(upload == "true")
            uploadGist("v2ray", upload_path, output_content, false);
    }
    else if(target == "quan")
    {
        std::cerr<<"Quantumult"<<std::endl;
        if(!ext.nodelist)
        {
            if(fileExist(ext_quan_base))
                base_content = fileGet(ext_quan_base, false);
            else
                base_content = webGet(ext_quan_base, getSystemProxy());
        }

        output_content = netchToQuan(nodes, base_content, rca, extra_group, ext);

        if(upload == "true")
            uploadGist("quan", upload_path, output_content, false);
    }
    else if(target == "quanx")
    {
        std::cerr<<"Quantumult X"<<std::endl;
        if(!ext.nodelist)
        {
            if(fileExist(ext_quanx_base))
                base_content = fileGet(ext_quanx_base, false);
            else
                base_content = webGet(ext_quanx_base, getSystemProxy());
        }

        output_content = netchToQuanX(nodes, base_content, rca, extra_group, ext);

        if(upload == "true")
            uploadGist("quanx", upload_path, output_content, false);
    }
    else if(target == "ssd")
    {
        std::cerr<<"SSD"<<std::endl;
        output_content = netchToSSD(nodes, group, ext);
        if(upload == "true")
            uploadGist("ssd", upload_path, output_content, false);
    }
    else
    {
        std::cerr<<"Unspecified"<<std::endl;
    }
    return output_content;
}

std::string simpleToClashR(RESPONSE_CALLBACK_ARGS)
{
    std::string url = argument.size() <= 8 ? "" : argument.substr(8);
    std::string base_content, output_content;
    std::vector<nodeInfo> nodes;
    string_array extra_group, extra_ruleset, include_remarks, exclude_remarks;
    std::vector<ruleset_content> rca;
    std::string subInfo;

    if(!url.size() && !api_mode)
        url = default_url;
    if(!url.size() || argument.substr(0, 8) != "sublink=")
    {
        *status_code = 400;
        return "Invalid request!";
    }
    if(url == "sublink")
    {
        *status_code = 400;
        return "Please insert your subscription link instead of clicking the default link.";
    }
    if(!api_mode || cfw_child_process)
        readConf();

    extra_group = clash_extra_group;

    if(update_ruleset_on_request || cfw_child_process)
        refreshRulesets(rulesets, ruleset_content_array);
    rca = ruleset_content_array;

    extra_settings ext = {true, overwrite_original_rules, safe_get_renames(), safe_get_emojis(), add_emoji, remove_old_emoji, append_proxy_type, udp_flag, tfo_flag, false, do_sort, scv_flag, filter_deprecated, ""};

    std::string proxy;
    if(proxy_subscription == "SYSTEM")
        proxy = getSystemProxy();
    else if(proxy_subscription == "NONE")
        proxy = "";
    else
        proxy = proxy_subscription;

    include_remarks = def_include_remarks;
    exclude_remarks = def_exclude_remarks;

    //start parsing urls
    int groupID = 0;
    string_array dummy;
    string_array urls = split(url, "|");
    for(std::string &x : urls)
    {
        x = trim(x);
        std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        if(addNodes(x, nodes, groupID, proxy, exclude_remarks, include_remarks, dummy, dummy, subInfo) == -1)
        {
            *status_code = 400;
            return std::string("The following link doesn't contain any valid node info: " + x);
        }
        groupID++;
    }
    //exit if found nothing
    if(!nodes.size())
    {
        *status_code = 400;
        return "No nodes were found!";
    }

    std::cerr<<"Generate target: ClashR\n";

    YAML::Node yamlnode = clash_base;
    netchToClash(nodes, yamlnode, extra_group, true, ext);
    return YAML::Dump(yamlnode);
}

std::string surgeConfToClash(RESPONSE_CALLBACK_ARGS)
{
    INIReader ini;
    YAML::Node clash = clash_base;
    string_array dummy_str_array;
    std::vector<nodeInfo> nodes;
    std::string base_content, url = argument.size() <= 5 ? "" : argument.substr(5);
    ini.store_any_line = true;

    if(!url.size())
        url = default_url;
    if(!url.size() || argument.substr(0, 5) != "link=")
    {
        *status_code = 400;
        return "Invalid request!";
    }
    if(url == "link")
    {
        *status_code = 400;
        return "Please insert your subscription link instead of clicking the default link.";
    }

    if(fileExist(url))
        base_content = fileGet(url, false);
    else
        base_content = webGet(url, getSystemProxy());

    if(ini.Parse(base_content) != INIREADER_EXCEPTION_NONE)
    {
        std::string errmsg = "Parsing Surge config failed! Reason: " + ini.GetLastError();
        std::cerr<<errmsg<<"\n";
        *status_code = 400;
        return errmsg;
    }
    if(!ini.SectionExist("Proxy") || !ini.SectionExist("Proxy Group") || !ini.SectionExist("Rule"))
    {
        std::string errmsg = "Incomplete surge config! Missing critical sections!";
        std::cerr<<errmsg<<"\n";
        *status_code = 400;
        return errmsg;
    }

    std::string proxy;
    if(proxy_subscription == "SYSTEM")
        proxy = getSystemProxy();
    else if(proxy_subscription == "NONE")
        proxy = "";
    else
        proxy = proxy_subscription;


    std::string subInfo;
    std::cerr<<"Fetching node data from url '"<<url<<"'."<<std::endl;
    if(addNodes(url, nodes, 0, proxy, dummy_str_array, dummy_str_array, dummy_str_array, dummy_str_array, subInfo) == -1)
    {
        *status_code = 400;
        return std::string("The following link doesn't contain any valid node info: " + url);
    }

    //exit if found nothing
    if(!nodes.size())
    {
        *status_code = 400;
        return "No nodes were found!";
    }

    extra_settings ext = {true, true, dummy_str_array, dummy_str_array, false, false, false, udp_flag, tfo_flag, false, do_sort, scv_flag, filter_deprecated, ""};

    netchToClash(nodes, clash, dummy_str_array, false, ext);

    string_multimap section;
    ini.GetItems("Proxy Group", section);
    std::string name, type, content;
    YAML::Node singlegroup;
    for(auto &x : section)
    {
        singlegroup.reset();
        name = x.first;
        content = x.second;
        dummy_str_array = split(content, ",");
        if(!dummy_str_array.size())
            continue;
        type = dummy_str_array[0];
        if(!(type == "select" || type == "url-test" || type == "fallback" || type == "load-balance")) //remove unsupported types
            continue;
        singlegroup["name"] = name;
        singlegroup["type"] = type;
        for(unsigned int i = 1; i < dummy_str_array.size(); i++)
        {
            if(dummy_str_array[i].find("url") == 0)
                singlegroup["url"] = trim(dummy_str_array[i].substr(dummy_str_array[i].find("=") + 1));
            else if(dummy_str_array[i].find("interval") == 0)
                singlegroup["interval"] = trim(dummy_str_array[i].substr(dummy_str_array[i].find("=") + 1));
            else
                singlegroup["proxies"].push_back(trim(dummy_str_array[i]));
        }
        clash["Proxy Group"].push_back(singlegroup);
    }
    section.clear();
    ini.GetItems("Proxy", section);
    for(auto &x : section)
    {
        singlegroup.reset();
        name = x.first;
        content = x.second;
        dummy_str_array = split(content, ",");
        if(!dummy_str_array.size())
            continue;
        content = trim(dummy_str_array[0]);
        if(content == "direct")
        {
            singlegroup["name"] = name;
            singlegroup["type"] = "select";
            singlegroup["proxies"].push_back("DIRECT");
        }
        else if(content == "reject")
        {
            singlegroup["name"] = name;
            singlegroup["type"] = "select";
            singlegroup["proxies"].push_back("REJECT");
        }
        else
            continue;
        clash["Proxy Group"].push_back(singlegroup);
    }

    eraseElements(dummy_str_array);
    ini.GetAll("Rule", "{NONAME}", dummy_str_array);
    YAML::Node rule;
    string_array strArray;
    std::string strLine;
    std::stringstream ss;
    std::string::size_type lineSize;
    for(std::string &x : dummy_str_array)
    {
        if(x.find("USER-AGENT") == 0 || x.find("URL-REGEX") == 0 || x.find("PROCESS-NAME") == 0 || x.find("AND") == 0 || x.find("OR") == 0) //remove unsupported types
            continue;
        else if(x.find("RULE-SET") == 0)
        {
            strArray = split(x, ",");
            if(strArray.size() != 3)
                continue;
            content = webGet(strArray[1], "");
            if(!content.size())
                continue;

            ss << content;
            char delimiter = count(content.begin(), content.end(), '\n') < 1 ? '\r' : '\n';

            while(getline(ss, strLine, delimiter))
            {
                lineSize = strLine.size();
                if(strLine[lineSize - 1] == '\r') //remove line break
                {
                    strLine = strLine.substr(0, lineSize - 1);
                    lineSize--;
                }
                if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                    continue;
                else if(strLine.find("USER-AGENT") == 0 || strLine.find("URL-REGEX") == 0 || strLine.find("PROCESS-NAME") == 0 || strLine.find("AND") == 0 || strLine.find("OR") == 0) //remove unsupported types
                    continue;
                strLine += strArray[2];
                if(std::count(strLine.begin(), strLine.end(), ',') > 2)
                    strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
                rule.push_back(strLine);
            }
            ss.clear();
            continue;
        }
        rule.push_back(x);
    }
    clash["Rule"] = rule;

    return YAML::Dump(clash);
}
