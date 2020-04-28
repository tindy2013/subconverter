#include <iostream>
#include <string>
#include <mutex>
#include <numeric>

#include <inja.hpp>
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
#include "string_hash.h"
#include "templates.h"
#include "upload.h"

//common settings
std::string pref_path = "pref.ini", def_ext_config;
string_array def_exclude_remarks, def_include_remarks, rulesets, stream_rules, time_rules;
std::vector<ruleset_content> ruleset_content_array;
std::string listen_address = "127.0.0.1", default_url, insert_url, managed_config_prefix;
int listen_port = 25500, max_pending_connections = 10, max_concurrent_threads = 4;
bool api_mode = true, write_managed_config = false, enable_rule_generator = true, update_ruleset_on_request = false, overwrite_original_rules = true;
bool print_debug_info = false, cfw_child_process = false, append_userinfo = true, enable_base_gen = false, async_fetch_ruleset = false;
std::string access_token, base_path = "base";
extern std::string custom_group;
extern int global_log_level;
string_map aliases_map;

//global variables for template
std::string template_path = "templates";
string_map global_vars;

//generator settings
bool generator_mode = false;
std::string gen_profile;

//multi-thread lock
std::mutex on_configuring;

//preferences
string_array renames, emojis;
bool add_emoji = false, remove_old_emoji = false, append_proxy_type = false, filter_deprecated = true;
bool udp_flag = false, tfo_flag = false, scv_flag = false, do_sort = false, config_update_strict = false;
bool clash_use_new_field_name = false;
std::string proxy_config, proxy_ruleset, proxy_subscription;
int config_update_interval = 0;

std::string clash_rule_base;
string_array clash_extra_group;
std::string surge_rule_base, surfboard_rule_base, mellow_rule_base, quan_rule_base, quanx_rule_base, loon_rule_base, sssub_rule_base;
std::string surge_ssr_path, quanx_script_id;

//pre-compiled rule bases
YAML::Node clash_base;
INIReader surge_base, mellow_base;

//cache system
bool serve_cache_on_fetch_fail = false;
int cache_subscription = 60, cache_config = 300, cache_ruleset = 21600;

string_array regex_blacklist = {"(.*)*"};

template <typename T> void operator >> (const YAML::Node& node, T& i)
{
    if(node.IsDefined() && !node.IsNull()) //fail-safe
        i = node.as<T>();
};

template <typename T> T safe_as (const YAML::Node& node)
{
    if(node.IsDefined() && !node.IsNull())
        return node.as<T>();
    return T();
};

std::string parseProxy(const std::string &source)
{
    std::string proxy = source;
    if(source == "SYSTEM")
        proxy = getSystemProxy();
    else if(source == "NONE")
        proxy = "";
    return proxy;
}

#define basic_types "DOMAIN", "DOMAIN-SUFFIX", "DOMAIN-KEYWORD", "IP-CIDR", "SRC-IP-CIDR", "GEOIP", "MATCH", "FINAL"
const string_array surge_rule_type = {basic_types, "IP-CIDR6", "USER-AGENT", "URL-REGEX", "AND", "OR", "NOT", "PROCESS-NAME", "IN-PORT", "DEST-PORT", "SRC-IP"};
const string_array quanx_rule_type = {basic_types, "USER-AGENT", "HOST", "HOST-SUFFIX", "HOST-KEYWORD"};

std::string getRuleset(RESPONSE_CALLBACK_ARGS)
{
    /// type: 1 for Surge, 2 for Quantumult X, 3 for Clash domain rule-provider, 4 for Clash ipcidr rule-provider, 5 for Surge DOMAIN-SET
    std::string url = urlsafe_base64_decode(getUrlArg(argument, "url")), type = getUrlArg(argument, "type"), group = urlsafe_base64_decode(getUrlArg(argument, "group"));
    std::string output_content, dummy;
    int type_int = to_int(type, 0);

    if(!url.size() || !type.size() || (type_int == 2 && !group.size()) || (type_int < 1 && type_int > 5))
    {
        *status_code = 400;
        return "Invalid request!";
    }

    std::string proxy = parseProxy(proxy_ruleset);
    string_array vArray = split(url, "|");
    for(std::string &x : vArray)
        output_content += fetchFile(x, proxy, cache_ruleset) + "\n";

    if(!output_content.size())
    {
        *status_code = 400;
        return "Invalid request!";
    }

    std::string strLine;
    std::stringstream ss;
    const std::string rule_match_regex = "^(.*?,.*?)(,.*)(,.*)$";

    ss << output_content;
    char delimiter = count(output_content.begin(), output_content.end(), '\n') < 1 ? '\r' : '\n';
    std::string::size_type lineSize;

    output_content.clear();

    if(type_int == 3 || type_int == 4)
        output_content = "payload:\n";

    while(getline(ss, strLine, delimiter))
    {
        switch(type_int)
        {
        case 2:
            if(!std::any_of(quanx_rule_type.begin(), quanx_rule_type.end(), [&strLine](std::string type){return startsWith(strLine, type);}) || startsWith(strLine, "IP-CIDR6"))
                continue;
            break;
        case 1:
            if(!std::any_of(surge_rule_type.begin(), surge_rule_type.end(), [&strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            break;
        case 3:
            if(!startsWith(strLine, "DOMAIN-SUFFIX,") && !startsWith(strLine, "DOMAIN,"))
                continue;
            vArray = split(strLine, ",");
            if(vArray.size() < 2)
                continue;
            vArray[1] = regTrim(vArray[1]);
            switch(hash_(vArray[0]))
            {
            case "DOMAIN-SUFFIX"_hash:
                strLine = "  - '." + vArray[1] + "'";
                break;
            case "DOMAIN"_hash:
                strLine = "  - '" + vArray[1] + "'";
                break;
            //case "DOMAIN_KEYWORD"_hash:
                //strLine = "  - '." + vArray[1] + ".*'";
                //break;
            }
            output_content += strLine + "\n";
            continue;
        case 4:
            if(!startsWith(strLine, "IP-CIDR,") && !startsWith(strLine, "IP-CIDR6,"))
                continue;
            vArray = split(strLine, ",");
            if(vArray.size() < 2)
                continue;
            output_content += "  - '" + vArray[1] + "'\n";
            continue;
        case 5:
            if(!startsWith(strLine, "DOMAIN-SUFFIX,") && !startsWith(strLine, "DOMAIN,"))
                continue;
            vArray = split(strLine, ",");
            if(vArray.size() < 2)
                continue;
            vArray[1] = regTrim(vArray[1]);
            switch(hash_(vArray[0]))
            {
            case "DOMAIN-SUFFIX"_hash:
            case "DOMAIN"_hash:
                strLine = vArray[1];
                break;
            //case "DOMAIN_KEYWORD"_hash:
                //strLine = "  - '." + vArray[1] + ".*'";
                //break;
            }
            output_content += strLine + "\n";
            continue;
        }

        lineSize = strLine.size();
        if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
        {
            strLine.erase(lineSize - 1);
            lineSize--;
        }

        if(!strLine.empty() && (strLine[0] != ';' && strLine[0] != '#' && !(lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')))
        {
            if(type_int == 2)
            {
                strLine += "," + group;
                if(std::count(strLine.begin(), strLine.end(), ',') > 2 && regReplace(strLine, rule_match_regex, "$2") == ",no-resolve")
                    strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                else
                    strLine = regReplace(strLine, rule_match_regex, "$1$3");
            }
        }

        output_content.append(strLine + "\n");
    }

    if(type_int == 3 && output_content == "payload:\n")
        output_content += "  - '--placeholder--'";
    if(type_int == 4 && output_content == "payload:\n")
        output_content += "  - '0.0.0.0/32'";
    return output_content;
}

int importItems(string_array &target, bool scope_limit = true)
{
    string_array result;
    std::stringstream ss;
    std::string path, content, strLine, dummy;
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

        std::string proxy = parseProxy(proxy_config);

        if(fileExist(path))
            content = fileGet(path, scope_limit);
        else if(startsWith(path, "http://") || startsWith(path, "https://") || startsWith(path, "data:"))
            content = webGet(path, proxy, dummy, cache_config);
        else
            writeLog(0, "File not found or not a valid URL: " + path, LOG_LEVEL_ERROR);
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

void readRegexMatch(YAML::Node node, std::string delimiter, string_array &dest, bool scope_limit = true)
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
    importItems(dest, scope_limit);
}

void readEmoji(YAML::Node node, string_array &dest, bool scope_limit = true)
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
    importItems(dest, scope_limit);
}

void readGroup(YAML::Node node, string_array &dest, bool scope_limit = true)
{
    string_array tempArray;
    YAML::Node object;
    unsigned int i, j;

    for(i = 0; i < node.size(); i++)
    {
        std::string strLine, name, type;
        name.clear();
        eraseElements(tempArray);
        object = node[i];
        object["import"] >> name;
        if(name.size())
        {
            name = "!!import:" + name;
            dest.emplace_back(name);
            continue;
        }
        std::string url = "http://www.gstatic.com/generate_204", interval = "300", tolerance, timeout;
        object["name"] >> name;
        object["type"] >> type;
        tempArray.emplace_back(name);
        tempArray.emplace_back(type);
        object["url"] >> url;
        object["interval"] >> interval;
        object["tolerance"] >> tolerance;
        object["timeout"] >> timeout;
        for(j = 0; j < object["rule"].size(); j++)
            tempArray.emplace_back(safe_as<std::string>(object["rule"][j]));
        if(type != "select" && type != "ssid")
        {
            tempArray.emplace_back(url);
            tempArray.emplace_back(interval + "," + timeout + "," + tolerance);
        }

        if((type == "select" && tempArray.size() < 3) || (type == "ssid" && tempArray.size() < 4) || (type != "select" && type != "ssid" && tempArray.size() < 5))
            continue;

        strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b) -> std::string
        {
            return std::move(a) + "`" + std::move(b);
        });
        dest.emplace_back(strLine);
    }
    importItems(dest, scope_limit);
}

void readRuleset(YAML::Node node, string_array &dest, bool scope_limit = true)
{
    std::string strLine, name, url, group;
    YAML::Node object;

    for(unsigned int i = 0; i < node.size(); i++)
    {
        url.clear();
        name.clear();
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
    importItems(dest, scope_limit);
}

void refreshRulesets(string_array &ruleset_list, std::vector<ruleset_content> &rca)
{
    eraseElements(rca);
    std::string rule_group, rule_url, dummy;
    ruleset_content rc;

    std::string proxy = parseProxy(proxy_ruleset);

    for(std::string &x : ruleset_list)
    {
        /*
        vArray = split(x, ",");
        if(vArray.size() != 2)
            continue;
        rule_group = trim(vArray[0]);
        rule_url = trim(vArray[1]);
        */
        string_size pos = x.find(",");
        if(pos == x.npos)
            continue;
        rule_group = trim(x.substr(0, pos));
        rule_url = trim(x.substr(pos + 1));
        if(rule_url.find("[]") == 0)
        {
            writeLog(0, "Adding rule '" + rule_url.substr(2) + "," + rule_group + "'.", LOG_LEVEL_INFO);
            //std::cerr<<"Adding rule '"<<rule_url.substr(2)<<","<<rule_group<<"'."<<std::endl;
            rc = {rule_group, "", std::async(std::launch::async, [rule_url](){return rule_url;})};
            rca.emplace_back(rc);
            continue;
        }
        else
        {
            //std::cerr<<"Updating ruleset url '"<<rule_url<<"' with group '"<<rule_group<<"'."<<std::endl;
            writeLog(0, "Updating ruleset url '" + rule_url + "' with group '" + rule_group + "'.", LOG_LEVEL_INFO);
            /*
            if(fileExist(rule_url))
            {
                rc = {rule_group, rule_url, fileGet(rule_url)};
            }
            else if(startsWith(rule_url, "https://") || startsWith(rule_url, "http://") || startsWith(rule_url, "data:"))
            {
                rc = {rule_group, rule_url, webGet(rule_url, proxy, dummy, cache_ruleset)};
            }
            else
                continue;
            */
            rc = {rule_group, rule_url, fetchFileAsync(rule_url, proxy, cache_ruleset, async_fetch_ruleset)};
        }
        rca.emplace_back(rc);
        /*
        if(rc.rule_content.get().size())
            rca.emplace_back(rc);
        else
            //std::cerr<<"Warning: No data was fetched from this link. Skipping..."<<std::endl;
            writeLog(0, "Warning: No data was fetched from ruleset '" + rule_url + "'. Skipping...", LOG_LEVEL_WARNING);
        */
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
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(a) + "|" + std::move(b);
            });
            default_url = strLine;
            eraseElements(tempArray);
        }
    }
    if(section["insert_url"].IsSequence())
    {
        section["insert_url"] >> tempArray;
        if(tempArray.size())
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(b) + "|" + std::move(a); // add in reverse order
            });
            insert_url = strLine;
            eraseElements(tempArray);
        }
    }
    if(section["exclude_remarks"].IsSequence())
        section["exclude_remarks"] >> def_exclude_remarks;
    if(section["include_remarks"].IsSequence())
        section["include_remarks"] >> def_include_remarks;
    section["base_path"] >> base_path;
    section["clash_rule_base"] >> clash_rule_base;
    section["surge_rule_base"] >> surge_rule_base;
    section["surfboard_rule_base"] >> surfboard_rule_base;
    section["mellow_rule_base"] >> mellow_rule_base;
    section["quan_rule_base"] >> quan_rule_base;
    section["quanx_rule_base"] >> quanx_rule_base;
    section["loon_rule_base"] >> loon_rule_base;
    section["sssub_rule_base"] >> sssub_rule_base;

    section["default_external_config"] >> def_ext_config;
    section["append_proxy_type"] >> append_proxy_type;
    section["proxy_config"] >> proxy_config;
    section["proxy_ruleset"] >> proxy_ruleset;
    section["proxy_subscription"] >> proxy_subscription;

    if(node["userinfo"].IsDefined())
    {
        section = node["userinfo"];
        if(section["stream_rule"].IsSequence())
        {
            readRegexMatch(section["stream_rule"], "|", tempArray, false);
            safe_set_streams(tempArray);
            eraseElements(tempArray);
        }
        if(section["time_rule"].IsSequence())
        {
            readRegexMatch(section["time_rule"], "|", tempArray, false);
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
        section["filter_deprecated_nodes"] >> filter_deprecated;
        section["append_sub_userinfo"] >> append_userinfo;
        section["clash_use_new_field_name"] >> clash_use_new_field_name;
    }

    if(section["rename_node"].IsSequence())
    {
        readRegexMatch(section["rename_node"], "@", tempArray, false);
        safe_set_renames(tempArray);
        eraseElements(tempArray);
    }

    if(node["managed_config"].IsDefined())
    {
        section = node["managed_config"];
        section["write_managed_config"] >> write_managed_config;
        section["managed_config_prefix"] >> managed_config_prefix;
        section["config_update_interval"] >> config_update_interval;
        section["config_update_strict"] >> config_update_strict;
        section["quanx_device_id"] >> quanx_script_id;
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
            readEmoji(section["rules"], tempArray, false);
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
            readRuleset(section["surge_ruleset"], rulesets, false);
    }

    if(node["proxy_group"].IsDefined() && node["proxy_group"]["custom_proxy_group"].IsDefined())
        readGroup(node["proxy_group"]["custom_proxy_group"], clash_extra_group, false);

    if(node["template"].IsDefined())
    {
        node["template"]["template_path"] >> template_path;
        if(node["template"]["globals"].IsSequence())
        {
            eraseElements(global_vars);
            for(size_t i = 0; i < node["template"]["globals"].size(); i++)
            {
                std::string key, value;
                node["template"]["globals"][i]["key"] >> key;
                node["template"]["globals"][i]["value"] >> value;
                global_vars[key] = value;
            }
        }
    }

    if(node["aliases"].IsSequence())
    {
        reset_redirect();
        for(size_t i = 0; i < node["aliases"].size(); i++)
        {
            std::string uri, target;
            node["aliases"][i]["uri"] >> uri;
            node["aliases"][i]["target"] >> target;
            append_redirect(uri, target);
        }
    }

    if(node["server"].IsDefined())
    {
        node["server"]["listen"] >> listen_address;
        node["server"]["port"] >> listen_port;
    }

    if(node["advanced"].IsDefined())
    {
        std::string log_level;
        node["advanced"]["log_level"] >> log_level;
        node["advanced"]["print_debug_info"] >> print_debug_info;
        if(print_debug_info)
            global_log_level = LOG_LEVEL_VERBOSE;
        else
        {
            switch(hash_(log_level))
            {
            case "warn"_hash:
                global_log_level = LOG_LEVEL_WARNING;
                break;
            case "error"_hash:
                global_log_level = LOG_LEVEL_ERROR;
                break;
            case "fatal"_hash:
                global_log_level = LOG_LEVEL_FATAL;
                break;
            case "verbose"_hash:
                global_log_level = LOG_LEVEL_VERBOSE;
                break;
            case "debug"_hash:
                global_log_level = LOG_LEVEL_DEBUG;
                break;
            default:
                global_log_level = LOG_LEVEL_INFO;
            }
        }
        node["advanced"]["max_pending_connections"] >> max_pending_connections;
        node["advanced"]["max_concurrent_threads"] >> max_concurrent_threads;
        node["advanced"]["enable_base_gen"] >> enable_base_gen;
        if(node["advanced"]["enable_cache"].IsDefined())
        {
            if(safe_as<bool>(node["advanced"]["enable_cache"]))
            {
                node["advanced"]["cache_subscription"] >> cache_subscription;
                node["advanced"]["cache_config"] >> cache_config;
                node["advanced"]["cache_ruleset"] >> cache_ruleset;
                node["advanced"]["serve_cache_on_fetch_fail"] >> serve_cache_on_fetch_fail;
            }
            else
                cache_subscription = cache_config = cache_ruleset = 0; //disable cache
        }
        node["advanced"]["async_fetch_ruleset"] >> async_fetch_ruleset;
    }
}

void readConf()
{
    guarded_mutex guard(on_configuring);
    //std::cerr<<"Reading preference settings..."<<std::endl;
    writeLog(0, "Reading preference settings...", LOG_LEVEL_INFO);

    eraseElements(def_exclude_remarks);
    eraseElements(def_include_remarks);
    eraseElements(clash_extra_group);
    eraseElements(rulesets);

    try
    {
        std::string prefdata = fileGet(pref_path, false);
        if(prefdata.find("common:") != prefdata.npos)
        {
            YAML::Node yaml = YAML::Load(prefdata);
            if(yaml.size() && yaml["common"])
                return readYAMLConf(yaml);
        }
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
        //std::cerr<<"Unable to load preference settings. Reason: "<<ini.GetLastError()<<"\n";
        writeLog(0, "Unable to load preference settings. Reason: " + ini.GetLastError(), LOG_LEVEL_FATAL);
        return;
    }

    string_array tempArray;

    ini.EnterSection("common");
    ini.GetBoolIfExist("api_mode", api_mode);
    ini.GetIfExist("api_access_token", access_token);
    ini.GetIfExist("default_url", default_url);
    ini.GetIfExist("insert_url", insert_url);
    tempArray = split(insert_url, "|");
    if(tempArray.size())
        insert_url = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
        {
            return std::move(b) + "|" + std::move(a);
        });
    if(ini.ItemPrefixExist("exclude_remarks"))
        ini.GetAll("exclude_remarks", def_exclude_remarks);
    if(ini.ItemPrefixExist("include_remarks"))
        ini.GetAll("include_remarks", def_include_remarks);
    ini.GetIfExist("base_path", base_path);
    ini.GetIfExist("clash_rule_base", clash_rule_base);
    ini.GetIfExist("surge_rule_base", surge_rule_base);
    ini.GetIfExist("surfboard_rule_base", surfboard_rule_base);
    ini.GetIfExist("mellow_rule_base", mellow_rule_base);
    ini.GetIfExist("quan_rule_base", quan_rule_base);
    ini.GetIfExist("quanx_rule_base", quanx_rule_base);
    ini.GetIfExist("loon_rule_base", loon_rule_base);
    ini.GetIfExist("default_external_config", def_ext_config);
    ini.GetBoolIfExist("append_proxy_type", append_proxy_type);
    ini.GetIfExist("proxy_config", proxy_config);
    ini.GetIfExist("proxy_ruleset", proxy_ruleset);
    ini.GetIfExist("proxy_subscription", proxy_subscription);

    if(ini.SectionExist("surge_external_proxy"))
    {
        ini.EnterSection("surge_external_proxy");
        ini.GetIfExist("surge_ssr_path", surge_ssr_path);
    }

    if(ini.SectionExist("node_pref"))
    {
        ini.EnterSection("node_pref");
        ini.GetBoolIfExist("udp_flag", udp_flag);
        ini.GetBoolIfExist("tcp_fast_open_flag", tfo_flag);
        ini.GetBoolIfExist("sort_flag", do_sort);
        ini.GetBoolIfExist("skip_cert_verify_flag", scv_flag);
        ini.GetBoolIfExist("filter_deprecated_nodes", filter_deprecated);
        ini.GetBoolIfExist("append_sub_userinfo", append_userinfo);
        ini.GetBoolIfExist("clash_use_new_field_name", clash_use_new_field_name);
        if(ini.ItemPrefixExist("rename_node"))
        {
            ini.GetAll("rename_node", tempArray);
            importItems(tempArray, false);
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
            importItems(tempArray, false);
            safe_set_streams(tempArray);
            eraseElements(tempArray);
        }
        if(ini.ItemPrefixExist("time_rule"))
        {
            ini.GetAll("time_rule", tempArray);
            importItems(tempArray, false);
            safe_set_times(tempArray);
            eraseElements(tempArray);
        }
    }

    ini.EnterSection("managed_config");
    ini.GetBoolIfExist("write_managed_config", write_managed_config);
    ini.GetIfExist("managed_config_prefix", managed_config_prefix);
    ini.GetIntIfExist("config_update_interval", config_update_interval);
    ini.GetBoolIfExist("config_update_strict", config_update_strict);
    ini.GetIfExist("quanx_device_id", quanx_script_id);

    ini.EnterSection("emojis");
    ini.GetBoolIfExist("add_emoji", add_emoji);
    ini.GetBoolIfExist("remove_old_emoji", remove_old_emoji);
    if(ini.ItemPrefixExist("rule"))
    {
        ini.GetAll("rule", tempArray);
        importItems(tempArray, false);
        safe_set_emojis(tempArray);
        eraseElements(tempArray);
    }

    ini.EnterSection("ruleset");
    enable_rule_generator = ini.GetBool("enabled");
    if(enable_rule_generator)
    {
        ini.GetBoolIfExist("overwrite_original_rules", overwrite_original_rules);
        ini.GetBoolIfExist("update_ruleset_on_request", update_ruleset_on_request);
        if(ini.ItemPrefixExist("surge_ruleset"))
        {
            ini.GetAll("surge_ruleset", rulesets);
            importItems(rulesets, false);
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
        importItems(clash_extra_group, false);
    }

    ini.EnterSection("template");
    ini.GetIfExist("template_path", template_path);
    string_multimap tempmap;
    ini.GetItems(tempmap);
    eraseElements(global_vars);
    for(auto &x : tempmap)
    {
        if(x.first == "template_path")
            continue;
        global_vars[x.first] = x.second;
    }
    global_vars["managed_config_prefix"] = managed_config_prefix;

    if(ini.SectionExist("aliases"))
    {
        ini.EnterSection("aliases");
        string_multimap tempmap;
        ini.GetItems(tempmap);
        reset_redirect();
        for(auto &x : tempmap)
            append_redirect(x.first, x.second);
    }

    ini.EnterSection("server");
    ini.GetIfExist("listen", listen_address);
    ini.GetIntIfExist("port", listen_port);

    ini.EnterSection("advanced");
    std::string log_level;
    ini.GetIfExist("log_level", log_level);
    ini.GetBoolIfExist("print_debug_info", print_debug_info);
    if(print_debug_info)
        global_log_level = LOG_LEVEL_VERBOSE;
    else
    {
        switch(hash_(log_level))
        {
        case "warn"_hash:
            global_log_level = LOG_LEVEL_WARNING;
            break;
        case "error"_hash:
            global_log_level = LOG_LEVEL_ERROR;
            break;
        case "fatal"_hash:
            global_log_level = LOG_LEVEL_FATAL;
            break;
        case "verbose"_hash:
            global_log_level = LOG_LEVEL_VERBOSE;
            break;
        case "debug"_hash:
            global_log_level = LOG_LEVEL_DEBUG;
            break;
        default:
            global_log_level = LOG_LEVEL_INFO;
        }
    }
    ini.GetIntIfExist("max_pending_connections", max_pending_connections);
    ini.GetIntIfExist("max_concurrent_threads", max_concurrent_threads);
    ini.GetBoolIfExist("enable_base_gen", enable_base_gen);
    if(ini.ItemExist("enable_cache"))
    {
        if(ini.GetBool("enable_cache"))
        {
            ini.GetIntIfExist("cache_subscription", cache_subscription);
            ini.GetIntIfExist("cache_config", cache_config);
            ini.GetIntIfExist("cache_ruleset", cache_ruleset);
            ini.GetBoolIfExist("serve_cache_on_fetch_fail", serve_cache_on_fetch_fail);
        }
        else
        {
            cache_subscription = cache_config = cache_ruleset = 0; //disable cache
            serve_cache_on_fetch_fail = false;
        }
    }
    ini.GetBoolIfExist("async_fetch_ruleset", async_fetch_ruleset);

    //std::cerr<<"Read preference settings completed."<<std::endl;
    writeLog(0, "Read preference settings completed.", LOG_LEVEL_INFO);
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
    std::string loon_rule_base;
    std::string sssub_rule_base;
    string_array rename;
    string_array emoji;
    string_array include;
    string_array exclude;
    template_args *tpl_args = NULL;
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
    section["loon_rule_base"] >> ext.loon_rule_base;
    section["sssub_rule_base"] >> ext.sssub_rule_base;

    section["enable_rule_generator"] >> ext.enable_rule_generator;
    section["overwrite_original_rules"] >> ext.overwrite_original_rules;

    if(section["custom_proxy_group"].size())
        readGroup(section["custom_proxy_group"], ext.custom_proxy_group, api_mode);

    if(section["surge_ruleset"].size())
        readRuleset(section["surge_ruleset"], ext.surge_ruleset, api_mode);

    if(section["rename_node"].size())
        readRegexMatch(section["rename_node"], "@", ext.rename, api_mode);

    section["include_remarks"] >> ext.include;
    section["exclude_remarks"] >> ext.exclude;

    if(node["template_args"].IsSequence() && ext.tpl_args != NULL)
    {
        std::string key, value;
        for(size_t i = 0; i < node["template_args"].size(); i++)
        {
            node["template_args"][i]["key"] >> key;
            node["template_args"][i]["value"] >> value;
            ext.tpl_args->local_vars[key] = value;
        }
    }

    return 0;
}

int loadExternalConfig(std::string &path, ExternalConfig &ext)
{
    std::string base_content, dummy, proxy = parseProxy(proxy_config), config = fetchFile(path, proxy, cache_config);
    if(render_template(config, *ext.tpl_args, base_content, template_path) != 0)
        base_content = config;

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
        //std::cerr<<"Load external configuration failed. Reason: "<<ini.GetLastError()<<"\n";
        writeLog(0, "Load external configuration failed. Reason: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return -1;
    }

    ini.EnterSection("custom");
    if(ini.ItemPrefixExist("custom_proxy_group"))
    {
        ini.GetAll("custom_proxy_group", ext.custom_proxy_group);
        importItems(ext.custom_proxy_group, api_mode);
    }
    if(ini.ItemPrefixExist("surge_ruleset"))
    {
        ini.GetAll("surge_ruleset", ext.surge_ruleset);
        importItems(ext.surge_ruleset, api_mode);
    }

    ini.GetIfExist("clash_rule_base", ext.clash_rule_base);
    ini.GetIfExist("surge_rule_base", ext.surge_rule_base);
    ini.GetIfExist("surfboard_rule_base", ext.surfboard_rule_base);
    ini.GetIfExist("mellow_rule_base", ext.mellow_rule_base);
    ini.GetIfExist("quan_rule_base", ext.quan_rule_base);
    ini.GetIfExist("quanx_rule_base", ext.quanx_rule_base);
    ini.GetIfExist("loon_rule_base", ext.loon_rule_base);
    ini.GetIfExist("sssub_rule_base", ext.sssub_rule_base);

    ini.GetBoolIfExist("overwrite_original_rules", ext.overwrite_original_rules);
    ini.GetBoolIfExist("enable_rule_generator", ext.enable_rule_generator);

    if(ini.ItemPrefixExist("rename"))
    {
        ini.GetAll("rename", ext.rename);
        importItems(ext.rename, api_mode);
    }
    if(ini.ItemPrefixExist("emoji"))
    {
        ini.GetAll("emoji", ext.emoji);
        importItems(ext.emoji, api_mode);
    }
    if(ini.ItemPrefixExist("include_remarks"))
        ini.GetAll("include_remarks", ext.include);
    if(ini.ItemPrefixExist("exclude_remarks"))
        ini.GetAll("exclude_remarks", ext.exclude);

    if(ini.SectionExist("template") && ext.tpl_args != NULL)
    {
        ini.EnterSection("template");
        string_multimap tempmap;
        ini.GetItems(tempmap);
        for(auto &x : tempmap)
            ext.tpl_args->local_vars[x.first] = x.second;
    }

    return 0;
}

void checkExternalBase(const std::string &path, std::string &dest)
{
    if(startsWith(path, "https://") || startsWith(path, "http://") || startsWith(path, "data:") || (startsWith(path, base_path) && fileExist(path)))
        dest = path;
}

void generateBase()
{
    if(!enable_base_gen)
        return;
    std::string base_content, proxy = parseProxy(proxy_config);
    //std::cerr<<"Generating base content for Clash/R...\n";
    writeLog(0, "Generating base content for Clash/R...", LOG_LEVEL_INFO);
    base_content = fetchFile(clash_rule_base, proxy, cache_config);

    try
    {
        clash_base = YAML::Load(base_content);
        rulesetToClash(clash_base, ruleset_content_array, overwrite_original_rules, clash_use_new_field_name);
    }
    catch (YAML::Exception &e)
    {
        //std::cerr<<"Unable to load Clash base content. Reason: "<<e.msg<<"\n";
        writeLog(0, "Unable to load Clash base content. Reason: " + e.msg, LOG_LEVEL_ERROR);
    }
    // mellow base generate removed for now
    /*
    std::cerr<<"Generating base content for Mellow...\n";
    base_content = fetchFile(mellow_rule_base, proxy, cache_config);
    mellow_base.keep_empty_section = true;
    mellow_base.store_any_line = true;
    if(mellow_base.Parse(base_content) != INIREADER_EXCEPTION_NONE)
        std::cerr<<"Unable to load Mellow base content. Reason: "<<mellow_base.GetLastError()<<"\n";
    else
        rulesetToSurge(mellow_base, ruleset_content_array, 0, overwrite_original_rules, std::string());
    */
}

std::string subconverter(RESPONSE_CALLBACK_ARGS)
{
    //check if we need to read configuration
    if((!api_mode || cfw_child_process) && !generator_mode)
        readConf();
    std::string target = getUrlArg(argument, "target"), url = UrlDecode(getUrlArg(argument, "url")), emoji = getUrlArg(argument, "emoji");
    std::string group = UrlDecode(getUrlArg(argument, "group")), upload = getUrlArg(argument, "upload"), upload_path = getUrlArg(argument, "upload_path"), version = getUrlArg(argument, "ver");
    std::string append_type = getUrlArg(argument, "append_type"), tfo = getUrlArg(argument, "tfo"), udp = getUrlArg(argument, "udp"), nodelist = getUrlArg(argument, "list");
    std::string include = UrlDecode(getUrlArg(argument, "include")), exclude = UrlDecode(getUrlArg(argument, "exclude")), sort_flag = getUrlArg(argument, "sort");
    std::string scv = getUrlArg(argument, "scv"), fdn = getUrlArg(argument, "fdn"), expand = getUrlArg(argument, "expand"), append_sub_userinfo = getUrlArg(argument, "append_info");
    std::string dev_id = getUrlArg(argument, "dev_id"), filename = getUrlArg(argument, "filename"), interval_str = getUrlArg(argument, "interval"), strict_str = getUrlArg(argument, "strict");
    std::string clash_new_field = getUrlArg(argument, "new_name"), clash_script = getUrlArg(argument, "script"), add_insert = getUrlArg(argument, "insert");
    std::string base_content, output_content;
    string_array extra_group, extra_ruleset, include_remarks = def_include_remarks, exclude_remarks = def_exclude_remarks;
    std::string groups = urlsafe_base64_decode(getUrlArg(argument, "groups")), ruleset = urlsafe_base64_decode(getUrlArg(argument, "ruleset")), config = UrlDecode(getUrlArg(argument, "config"));
    std::vector<ruleset_content> rca;
    extra_settings ext;
    std::string subInfo, dummy;
    int interval = interval_str.size() ? to_int(interval_str, config_update_interval) : config_update_interval;
    bool ruleset_updated = false, authorized = !api_mode || getUrlArg(argument, "token") == access_token, strict = strict_str.size() ? strict_str == "true" : config_update_strict;
    bool insert_flag = add_insert.empty() || add_insert == "true";

    if(std::find(regex_blacklist.cbegin(), regex_blacklist.cend(), include) != regex_blacklist.cend() || std::find(regex_blacklist.cbegin(), regex_blacklist.cend(), exclude) != regex_blacklist.cend())
        return "Invalid request!";

    //for external configuration
    std::string ext_clash_base = clash_rule_base, ext_surge_base = surge_rule_base, ext_mellow_base = mellow_rule_base, ext_surfboard_base = surfboard_rule_base;
    std::string ext_quan_base = quan_rule_base, ext_quanx_base = quanx_rule_base, ext_loon_base = loon_rule_base, ext_sssub_base = sssub_rule_base;

    //validate urls
    if(!url.size() && (!api_mode || authorized))
        url = default_url;
    if(insert_url.size() && insert_flag)
        url = insert_url + "|" + url;
    if(!url.size() || !target.size())
    {
        *status_code = 400;
        return "Invalid request!";
    }

    //load request arguments as template variables
    string_array req_args = split(argument, "&");
    string_size pos;
    string_map req_arg_map;
    for(std::string &x : req_args)
    {
        pos = x.find("=");
        if(pos == x.npos)
            continue;
        if(x.substr(0, pos) == "token")
            continue;
        req_arg_map[x.substr(0, pos)] = x.substr(pos + 1);
    }

    //save template variables
    template_args tpl_args;
    tpl_args.global_vars = global_vars;
    tpl_args.request_params = req_arg_map;

    //check for proxy settings
    std::string proxy = parseProxy(proxy_subscription);

    ext.emoji_array = safe_get_emojis();
    ext.rename_array = safe_get_renames();

    //check other flags
    if(emoji.size())
    {
        ext.add_emoji = emoji == "true";
        ext.remove_emoji = true;
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
    ext.clash_new_field_name = clash_new_field.size() ? clash_new_field == "true" : clash_use_new_field_name;
    ext.clash_script = clash_script == "true";

    ext.nodelist = nodelist == "true";
    ext.surge_ssr_path = surge_ssr_path;
    ext.quanx_dev_id = dev_id.size() ? dev_id : quanx_script_id;
    ext.enable_rule_generator = enable_rule_generator;
    ext.overwrite_original_rules = overwrite_original_rules;
    if(expand != "true")
        ext.managed_config_prefix = managed_config_prefix;

    //load external configuration
    if(config.empty())
        config = def_ext_config;
    if(config.size())
    {
        //std::cerr<<"External configuration file provided. Loading...\n";
        writeLog(0, "External configuration file provided. Loading...", LOG_LEVEL_INFO);
        //read predefined data first
        extra_group = clash_extra_group;
        extra_ruleset = rulesets;
        //then load external configuration
        ExternalConfig extconf;
        extconf.tpl_args = &tpl_args;
        loadExternalConfig(config, extconf);
        if(!ext.nodelist)
        {
            checkExternalBase(extconf.clash_rule_base, ext_clash_base);
            checkExternalBase(extconf.surge_rule_base, ext_surge_base);
            checkExternalBase(extconf.surfboard_rule_base, ext_surfboard_base);
            checkExternalBase(extconf.mellow_rule_base, ext_mellow_base);
            checkExternalBase(extconf.quan_rule_base, ext_quan_base);
            checkExternalBase(extconf.quanx_rule_base, ext_quanx_base);
            checkExternalBase(extconf.loon_rule_base, ext_loon_base);
            checkExternalBase(extconf.sssub_rule_base, ext_sssub_base);
        }
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
        if(extconf.surge_ruleset.size() && !ext.nodelist)
        {
            extra_ruleset = extconf.surge_ruleset;
            refreshRulesets(extra_ruleset, rca);
            ruleset_updated = true;
        }
        else
        {
            if(ext.enable_rule_generator && !ext.nodelist)
            {
                if(update_ruleset_on_request)
                    refreshRulesets(rulesets, ruleset_content_array);
                rca = ruleset_content_array;
            }
        }
        if(extconf.include.size())
            include_remarks = extconf.include;
        if(extconf.exclude.size())
            exclude_remarks = extconf.exclude;
    }
    else
    {
        //loading custom groups
        if(groups.size() && !ext.nodelist)
        {
            extra_group = split(groups, "@");
            if(!extra_group.size())
                extra_group = clash_extra_group;
        }
        else
            extra_group = clash_extra_group;

        //loading custom rulesets
        if(ruleset.size() && !ext.nodelist)
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
            if(enable_rule_generator && !ext.nodelist)
            {
                if(update_ruleset_on_request || cfw_child_process)
                    refreshRulesets(rulesets, ruleset_content_array);
                rca = ruleset_content_array;
            }
        }
    }

    //loading urls
    string_array urls = split(url, "|");
    std::vector<nodeInfo> nodes;
    int groupID = 0;
    groupID -= insert_url.empty() || !insert_flag ? 0 : std::count(insert_url.begin(), insert_url.end(), '|') + 1;

    //check custom include/exclude settings
    if(include.size() && regValid(include))
        include_remarks = string_array{include};
    if(exclude.size() && regValid(exclude))
        exclude_remarks = string_array{exclude};

    //start parsing urls
    string_array stream_temp = safe_get_streams(), time_temp = safe_get_times();
    for(std::string &x : urls)
    {
        x = regTrim(x);
        //std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
        if(addNodes(x, nodes, groupID, proxy, exclude_remarks, include_remarks, stream_temp, time_temp, subInfo, authorized) == -1)
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

    if(subInfo.size() && (append_sub_userinfo.size() ? append_sub_userinfo == "true" : append_userinfo))
        extra_headers.emplace("Subscription-UserInfo", subInfo);

    //do pre-process now
    preprocessNodes(nodes, ext);

    string_array dummy_group;
    std::vector<ruleset_content> dummy_ruleset;
    std::string managed_url = base64_decode(UrlDecode(getUrlArg(argument, "profile_data")));
    if(managed_url.empty())
        managed_url = managed_config_prefix + "/sub?" + argument;

    //std::cerr<<"Generate target: ";
    int surge_ver;
    proxy = parseProxy(proxy_config);
    switch(hash_(target))
    {
    case "clash"_hash: case "clashr"_hash:
        //std::cerr<<"Clash"<<((target == "clashr") ? "R" : "")<<std::endl;
        writeLog(0, target == "clashr" ? "Generate target: ClashR" : "Generate target: Clash", LOG_LEVEL_INFO);
        tpl_args.local_vars["clash.new_field_name"] = ext.clash_new_field_name ? "true" : "false";
        if(ext.nodelist)
        {
            YAML::Node yamlnode;
            netchToClash(nodes, yamlnode, dummy_group, target == "clashr", ext);
            output_content = YAML::Dump(yamlnode);
        }
        else if(ruleset_updated || update_ruleset_on_request || ext_clash_base != clash_rule_base || !enable_base_gen)
        {
            if(render_template(fetchFile(ext_clash_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            //base_content = fetchFile(ext_clash_base, proxy, cache_config);
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
        break;
    case "surge"_hash:
        surge_ver = version.size() ? to_int(version, 3) : 3;
        //std::cerr<<"Surge "<<surge_ver<<std::endl;
        writeLog(0, "Generate target: Surge " + std::to_string(surge_ver), LOG_LEVEL_INFO);

        if(ext.nodelist)
        {
            output_content = netchToSurge(nodes, base_content, dummy_ruleset, dummy_group, surge_ver, ext);

            if(upload == "true")
                uploadGist("surge" + version + "list", upload_path, output_content, true);
        }
        else
        {
            if(render_template(fetchFile(ext_surge_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            //base_content = fetchFile(ext_surge_base, proxy, cache_config);
            output_content = netchToSurge(nodes, base_content, rca, extra_group, surge_ver, ext);

            if(upload == "true")
                uploadGist("surge" + version, upload_path, output_content, true);

            if(write_managed_config && managed_config_prefix.size())
                output_content = "#!MANAGED-CONFIG " + managed_url + (interval ? " interval=" + std::to_string(interval) : "") \
                 + " strict=" + std::string(strict ? "true" : "false") + "\n\n" + output_content;
        }
        break;
    case "surfboard"_hash:
        //std::cerr<<"Surfboard"<<std::endl;
        writeLog(0, "Generate target: Surfboard", LOG_LEVEL_INFO);

        if(render_template(fetchFile(ext_surfboard_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        //base_content = fetchFile(ext_surfboard_base, proxy, cache_config);
        output_content = netchToSurge(nodes, base_content, rca, extra_group, -3, ext);
        if(upload == "true")
            uploadGist("surfboard", upload_path, output_content, true);

        if(write_managed_config && managed_config_prefix.size())
            output_content = "#!MANAGED-CONFIG " + managed_url + (interval ? " interval=" + std::to_string(interval) : "") \
                 + " strict=" + std::string(strict ? "true" : "false") + "\n\n" + output_content;
        break;
    case "mellow"_hash:
        //std::cerr<<"Mellow"<<std::endl;
        writeLog(0, "Generate target: Mellow", LOG_LEVEL_INFO);
        // mellow base generator removed for now
        //if(ruleset_updated || update_ruleset_on_request || ext_mellow_base != mellow_rule_base || !enable_base_gen)
        {
            if(render_template(fetchFile(ext_mellow_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            //base_content = fetchFile(ext_mellow_base, proxy, cache_config);
            output_content = netchToMellow(nodes, base_content, rca, extra_group, ext);
        }
        /*
        else
        {
            INIReader ini;
            ini = safe_get_mellow_base();
            netchToMellow(nodes, ini, rca, extra_group, ext);
            output_content = ini.ToString();
        }
        */

        if(upload == "true")
            uploadGist("mellow", upload_path, output_content, true);
        break;
    case "ss"_hash:
        //std::cerr<<"SS"<<std::endl;
        writeLog(0, "Generate target: SS", LOG_LEVEL_INFO);
        output_content = netchToSS(nodes, ext);
        if(upload == "true")
            uploadGist("ss", upload_path, output_content, false);
        break;
    case "sssub"_hash:
        //std::cerr<<"SS Subscription"<<std::endl;
        writeLog(0, "Generate target: SS Subscription", LOG_LEVEL_INFO);

        if(render_template(fetchFile(ext_sssub_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        //base_content = fetchFile(ext_sssub_base, proxy, cache_config);
        output_content = netchToSSSub(base_content, nodes, ext);
        if(upload == "true")
            uploadGist("sssub", upload_path, output_content, false);
        break;
    case "ssr"_hash:
        //std::cerr<<"SSR"<<std::endl;
        writeLog(0, "Generate target: SSR", LOG_LEVEL_INFO);
        output_content = netchToSSR(nodes, ext);
        if(upload == "true")
            uploadGist("ssr", upload_path, output_content, false);
        break;
    case "v2ray"_hash:
        //std::cerr<<"v2rayN"<<std::endl;
        writeLog(0, "Generate target: v2rayN", LOG_LEVEL_INFO);
        output_content = netchToVMess(nodes, ext);
        if(upload == "true")
            uploadGist("v2ray", upload_path, output_content, false);
        break;
    case "quan"_hash:
        //std::cerr<<"Quantumult"<<std::endl;
        writeLog(0, "Generate target: Quantumult", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(ext_quan_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            //base_content = fetchFile(ext_quan_base, proxy, cache_config);
        }

        output_content = netchToQuan(nodes, base_content, rca, extra_group, ext);

        if(upload == "true")
            uploadGist("quan", upload_path, output_content, false);
        break;
    case "quanx"_hash:
        //std::cerr<<"Quantumult X"<<std::endl;
        writeLog(0, "Generate target: Quantumult X", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(ext_quanx_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            //base_content = fetchFile(ext_quanx_base, proxy, cache_config);
        }


        output_content = netchToQuanX(nodes, base_content, rca, extra_group, ext);

        if(upload == "true")
            uploadGist("quanx", upload_path, output_content, false);
        break;
    case "loon"_hash:
        //std::cerr<<"Loon"<<std::endl;
        writeLog(0, "Generate target: Loon", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(ext_loon_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            //base_content = fetchFile(ext_loon_base, proxy, cache_config);
        }

        output_content = netchToLoon(nodes, base_content, rca, extra_group, ext);

        if(upload == "true")
            uploadGist("loon", upload_path, output_content, false);
        break;
    case "ssd"_hash:
        //std::cerr<<"SSD"<<std::endl;
        writeLog(0, "Generate target: SSD", LOG_LEVEL_INFO);
        output_content = netchToSSD(nodes, group, subInfo, ext);
        if(upload == "true")
            uploadGist("ssd", upload_path, output_content, false);
        break;
    case "trojan"_hash:
        //std::cerr<<"Trojan"<<std::endl;
        writeLog(0, "Generate target: Trojan", LOG_LEVEL_INFO);
        output_content = netchToTrojan(nodes, ext);
        if(upload == "true")
            uploadGist("trojan", upload_path, output_content, false);
        break;
    default:
        //std::cerr<<"Unspecified"<<std::endl;
        writeLog(0, "Generate target: Unspecified", LOG_LEVEL_INFO);
        *status_code = 500;
        return "Unrecognized target";
    }
    writeLog(0, "Generate completed.", LOG_LEVEL_INFO);
    if(filename.size())
        extra_headers.emplace("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    return output_content;
}

std::string simpleToClashR(RESPONSE_CALLBACK_ARGS)
{
    std::string url = argument.size() <= 8 ? "" : argument.substr(8);
    std::string base_content;
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
    if(insert_url.size())
        url = insert_url + "|" + url;
    if(!api_mode || cfw_child_process)
        readConf();

    extra_group = clash_extra_group;

    if(update_ruleset_on_request || cfw_child_process)
        refreshRulesets(rulesets, ruleset_content_array);
    rca = ruleset_content_array;

    extra_settings ext = {true, overwrite_original_rules, safe_get_renames(), safe_get_emojis(), add_emoji, remove_old_emoji, append_proxy_type, udp_flag, tfo_flag, false, do_sort, scv_flag, filter_deprecated, clash_use_new_field_name, "", "", ""};

    std::string proxy = parseProxy(proxy_subscription);

    include_remarks = def_include_remarks;
    exclude_remarks = def_exclude_remarks;

    //start parsing urls
    int groupID = 0;
    string_array dummy;
    string_array urls = split(url, "|");
    for(std::string &x : urls)
    {
        x = trim(x);
        //std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
        if(addNodes(x, nodes, groupID, proxy, exclude_remarks, include_remarks, dummy, dummy, subInfo, false) == -1)
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

    writeLog(0, "Generate target: ClashR", LOG_LEVEL_INFO);

    template_args tpl_args;
    tpl_args.global_vars = global_vars;
    tpl_args.local_vars["clash.new_field_name"] = clash_use_new_field_name ? "true" : "false";
    tpl_args.request_params["target"] = "clashr";
    tpl_args.request_params["url"] = url;

    if(!enable_base_gen)
    {
        if(render_template(fetchFile(clash_rule_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        //base_content = fetchFile(clash_rule_base, proxy, cache_config);
        return netchToClash(nodes, base_content, rca, extra_group, true, ext);
    }
    else
    {
        YAML::Node yamlnode = safe_get_clash_base();
        netchToClash(nodes, yamlnode, extra_group, true, ext);
        return YAML::Dump(yamlnode);
    }
}

std::string surgeConfToClash(RESPONSE_CALLBACK_ARGS)
{
    INIReader ini;
    string_array dummy_str_array;
    std::vector<nodeInfo> nodes;
    std::string base_content, url = argument.size() <= 5 ? "" : argument.substr(5), dummy;
    const std::string proxygroup_name = clash_use_new_field_name ? "proxy-groups" : "Proxy Group", rule_name = clash_use_new_field_name ? "rules" : "Rule";

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
    writeLog(0, "SurgeConfToClash called with url '" + url + "'.", LOG_LEVEL_INFO);

    std::string proxy = parseProxy(proxy_config);
    YAML::Node clash;
    template_args tpl_args;
    tpl_args.global_vars = global_vars;
    tpl_args.local_vars["clash.new_field_name"] = clash_use_new_field_name ? "true" : "false";
    tpl_args.request_params["target"] = "clash";
    tpl_args.request_params["url"] = url;

    if(!enable_base_gen)
    {
        if(render_template(fetchFile(clash_rule_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        //base_content = fetchFile(clash_rule_base, proxy, cache_config);
        clash = YAML::Load(base_content);
    }
    else
    {
        clash = safe_get_clash_base();
    }

    base_content = fetchFile(url, proxy, cache_config);

    if(ini.Parse(base_content) != INIREADER_EXCEPTION_NONE)
    {
        std::string errmsg = "Parsing Surge config failed! Reason: " + ini.GetLastError();
        //std::cerr<<errmsg<<"\n";
        writeLog(0, errmsg, LOG_LEVEL_ERROR);
        *status_code = 400;
        return errmsg;
    }
    if(!ini.SectionExist("Proxy") || !ini.SectionExist("Proxy Group") || !ini.SectionExist("Rule"))
    {
        std::string errmsg = "Incomplete surge config! Missing critical sections!";
        //std::cerr<<errmsg<<"\n";
        writeLog(0, errmsg, LOG_LEVEL_ERROR);
        *status_code = 400;
        return errmsg;
    }

    //scan groups first, get potential policy-path
    string_multimap section;
    ini.GetItems("Proxy Group", section);
    std::string name, type, content;
    string_array links;
    links.emplace_back(url);
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
            else if(dummy_str_array[i].find("policy-path") == 0)
                links.emplace_back(trim(dummy_str_array[i].substr(dummy_str_array[i].find("=") + 1)));
            else
                singlegroup["proxies"].push_back(trim(dummy_str_array[i]));
        }
        clash[proxygroup_name].push_back(singlegroup);
    }

    proxy = parseProxy(proxy_subscription);
    eraseElements(dummy_str_array);

    std::string subInfo;
    for(std::string &x : links)
    {
        //std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
        if(addNodes(x, nodes, 0, proxy, dummy_str_array, dummy_str_array, dummy_str_array, dummy_str_array, subInfo, !api_mode) == -1)
        {
            *status_code = 400;
            return std::string("The following link doesn't contain any valid node info: " + x);
        }
    }

    //exit if found nothing
    if(!nodes.size())
    {
        *status_code = 400;
        return "No nodes were found!";
    }

    extra_settings ext = {true, true, dummy_str_array, dummy_str_array, false, false, false, udp_flag, tfo_flag, false, do_sort, scv_flag, filter_deprecated, clash_use_new_field_name, "", "", ""};

    netchToClash(nodes, clash, dummy_str_array, false, ext);

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
        clash[proxygroup_name].push_back(singlegroup);
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
            content = webGet(strArray[1], proxy, dummy, cache_ruleset);
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
    clash[rule_name] = rule;

    writeLog(0, "Conversion completed.", LOG_LEVEL_INFO);
    return YAML::Dump(clash);
}

std::string getProfile(RESPONSE_CALLBACK_ARGS)
{
    std::string name = UrlDecode(getUrlArg(argument, "name")), token = UrlDecode(getUrlArg(argument, "token"));
    if(token.empty() || name.empty())
    {
        *status_code = 403;
        return "Forbidden";
    }
    if(!fileExist(name))
    {
        *status_code = 404;
        return "Profile not found";
    }
    //std::cerr<<"Trying to load profile '" + name + "'.\n";
    writeLog(0, "Trying to load profile '" + name + "'.", LOG_LEVEL_INFO);
    INIReader ini;
    if(ini.ParseFile(name) != INIREADER_EXCEPTION_NONE && !ini.SectionExist("Profile"))
    {
        //std::cerr<<"Load profile failed! Reason: "<<ini.GetLastError()<<"\n";
        writeLog(0, "Load profile failed! Reason: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        *status_code = 500;
        return "Broken profile!";
    }
    //std::cerr<<"Trying to parse profile '" + name + "'.\n";
    writeLog(0, "Trying to parse profile '" + name + "'.", LOG_LEVEL_INFO);
    string_multimap contents;
    ini.GetItems("Profile", contents);
    if(!contents.size())
    {
        //std::cerr<<"Load profile failed! Reason: Empty Profile section\n";
        writeLog(0, "Load profile failed! Reason: Empty Profile section", LOG_LEVEL_ERROR);
        *status_code = 500;
        return "Broken profile!";
    }
    auto profile_token = contents.find("profile_token");
    if(profile_token != contents.end())
    {
        if(token != profile_token->second)
        {
            *status_code = 403;
            return "Forbidden";
        }
        token = access_token;
    }
    else
    {
        if(token != access_token)
        {
            *status_code = 403;
            return "Forbidden";
        }
    }

    contents.emplace("token", token);
    contents.emplace("profile_data", base64_encode(managed_config_prefix + "/getprofile?" + argument));
    std::string query;
    for(auto &x : contents)
    {
        query += x.first + "=" + UrlEncode(x.second) + "&";
    }
    query += argument;
    return subconverter(query, postdata, status_code, extra_headers);
}

std::string getScript(RESPONSE_CALLBACK_ARGS)
{
    std::string url = urlsafe_base64_decode(getUrlArg(argument, "url")), dev_id = getUrlArg(argument, "id");
    std::string output_content, dummy;

    std::string proxy = parseProxy(proxy_config);

    output_content = fetchFile(url, proxy, cache_config);

    if(!dev_id.size())
        dev_id = quanx_script_id;

    const std::string pattern = "(\\/\\*[\\s\\S]*?)^(.*?@supported )(.*?\\s?)$([\\s\\S]*\\*\\/\\s?";
    if(dev_id.size())
    {
        if(regFind(output_content, pattern))
            output_content = regReplace(output_content, pattern, "$1$2" + dev_id + "$4");
        else
            output_content = "/**\n * @supported " + dev_id + "\n * THIS COMMENT IS GENERATED BY SUBCONVERTER\n */\n\n" + output_content;
    }
    return output_content;
}

std::string getRewriteRemote(RESPONSE_CALLBACK_ARGS)
{
    std::string url = urlsafe_base64_decode(getUrlArg(argument, "url")), dev_id = getUrlArg(argument, "id");
    std::string output_content, dummy;

    std::string proxy = parseProxy(proxy_config);

    output_content = fetchFile(url, proxy, cache_config);

    if(!dev_id.size())
        dev_id = quanx_script_id;

    if(dev_id.size())
    {
        std::stringstream ss;
        std::string strLine;
        const std::string pattern = "^(.*? url script-.*? )(.*?)$";
        string_size lineSize;
        char delimiter = count(output_content.begin(), output_content.end(), '\n') < 1 ? '\r' : '\n';

        ss << output_content;
        output_content.clear();
        while(getline(ss, strLine, delimiter))
        {
            lineSize = strLine.size();
            if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
            {
                strLine.erase(lineSize - 1);
                lineSize--;
            }

            if(!strLine.empty() && regMatch(strLine, pattern))
            {
                url = managed_config_prefix + "/qx-script?id=" + dev_id + "&url=" + urlsafe_base64_encode(regReplace(strLine, pattern, "$2"));
                strLine = regReplace(strLine, pattern, "$1") + url;
            }
            output_content.append(strLine + "\n");
        }
    }
    return output_content;
}

std::string parseHostname(inja::Arguments &args)
{
    std::string data = args.at(0)->get<std::string>();
    string_array urls = split(data, ",");
    if(!urls.size())
        return std::string();

    std::string input_content, output_content, proxy = parseProxy(proxy_config);
    for(std::string &x : urls)
    {
        input_content = webGet(x, proxy, cache_config);
        output_content += regReplace(input_content, "(?:[\\s\\S]*?)^(?i:hostname\\s*?=\\s*?)(.*?)\\s$(?:[\\s\\S]*)", "$1") + ",";
    }
    string_array vArray = split(output_content, ",");
    std::set<std::string> hostnames;
    for(std::string &x : vArray)
        hostnames.emplace(trim(x));
    output_content = std::accumulate(hostnames.begin(), hostnames.end(), std::string(), [](std::string a, std::string b)
    {
        return std::move(a) + "," + std::move(b);
    });
    return output_content;
}

std::string template_webGet(inja::Arguments &args)
{
    std::string data = args.at(0)->get<std::string>(), proxy = parseProxy(proxy_config);
    writeLog(0, "Template called fetch with url '" + data + "'.", LOG_LEVEL_INFO);
    return webGet(data, proxy, cache_config);
}

std::string jinja2_webGet(const std::string &url)
{
    std::string proxy = parseProxy(proxy_config);
    writeLog(0, "Template called fetch with url '" + url + "'.", LOG_LEVEL_INFO);
    return webGet(url, proxy, cache_config);
}

static inline std::string intToStream(unsigned long long stream)
{
    char chrs[16] = {}, units[6] = {' ', 'K', 'M', 'G', 'T', 'P'};
    double streamval = stream;
    unsigned int level = 0;
    while(streamval > 1024.0)
    {
        if(level >= 5)
            break;
        level++;
        streamval /= 1024.0;
    }
    snprintf(chrs, 15, "%.2f %cB", streamval, units[level]);
    return std::string(chrs);
}

std::string subInfoToMessage(std::string subinfo)
{
    typedef unsigned long long ull;
    subinfo = replace_all_distinct(subinfo, "; ", "&");
    std::string retdata, useddata = "N/A", totaldata = "N/A", expirydata = "N/A";
    std::string upload = getUrlArg(subinfo, "upload"), download = getUrlArg(subinfo, "download"), total = getUrlArg(subinfo, "total"), expire = getUrlArg(subinfo, "expire");
    ull used = to_number<ull>(upload, 0) + to_number<ull>(download, 0), tot = to_number<ull>(total, 0);
    time_t expiry = to_number<time_t>(expire, 0);
    if(used != 0)
        useddata = intToStream(used);
    if(tot != 0)
        totaldata = intToStream(tot);
    if(expiry != 0)
    {
        char buffer[30];
        struct tm *dt = localtime(&expiry);
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", dt);
        expirydata.assign(buffer);
    }
    if(useddata == "N/A" && totaldata == "N/A" && expirydata == "N/A")
        retdata = "Not Available";
    else
        retdata += "Stream Used: " + useddata + " Stream Total: " + totaldata + " Expiry Time: " + expirydata;
    return retdata;
}

int simpleGenerator()
{
    //std::cerr<<"\nReading generator configuration...\n";
    writeLog(0, "Reading generator configuration...", LOG_LEVEL_INFO);
    std::string config = fileGet("generate.ini"), path, profile, arguments, content;
    if(config.empty())
    {
        //std::cerr<<"Generator configuration not found or empty!\n";
        writeLog(0, "Generator configuration not found or empty!", LOG_LEVEL_ERROR);
        return -1;
    }

    INIReader ini;
    if(ini.Parse(config) != INIREADER_EXCEPTION_NONE)
    {
        //std::cerr<<"Generator configuration broken! Reason:"<<ini.GetLastError()<<"\n";
        writeLog(0, "Generator configuration broken! Reason:" + ini.GetLastError(), LOG_LEVEL_ERROR);
        return -2;
    }
    //std::cerr<<"Read generator configuration completed.\n\n";
    writeLog(0, "Read generator configuration completed.\n", LOG_LEVEL_INFO);

    string_array sections = ini.GetSections();
    if(gen_profile.size())
    {
        //std::cerr<<"Generating with specific artifacts: \""<<gen_profile<<"\"...\n";
        writeLog(0, "Generating with specific artifacts: \"" + gen_profile + "\"...", LOG_LEVEL_INFO);
        string_array targets = split(gen_profile, ","), new_targets;
        for(std::string &x : targets)
        {
            x = trim(x);
            if(std::find(sections.cbegin(), sections.cend(), x) != sections.cend())
                new_targets.emplace_back(x);
            else
            {
                //std::cerr<<"Artifact \""<<x<<"\" not found in generator settings!\n";
                writeLog(0, "Artifact \"" + x + "\" not found in generator settings!", LOG_LEVEL_ERROR);
                return -3;
            }
        }
        sections = new_targets;
        sections.shrink_to_fit();
    }
    else
        //std::cerr<<"Generating all artifacts...\n";
        writeLog(0, "Generating all artifacts...", LOG_LEVEL_INFO);

    string_multimap allItems;
    std::string dummy_str;
    std::string proxy = parseProxy(proxy_subscription);
    string_map headers;
    int ret_code = 200;
    for(std::string &x : sections)
    {
        arguments.clear();
        ret_code = 200;
        //std::cerr<<"Generating artifact '"<<x<<"'...\n";
        writeLog(0, "Generating artifact '" + x + "'...", LOG_LEVEL_INFO);
        ini.EnterSection(x);
        if(ini.ItemExist("path"))
            path = ini.Get("path");
        else
        {
            //std::cerr<<"Artifact '"<<x<<"' output path missing! Skipping...\n\n";
            writeLog(0, "Artifact '" + x + "' output path missing! Skipping...\n", LOG_LEVEL_ERROR);
            continue;
        }
        if(ini.ItemExist("profile"))
        {
            profile = ini.Get("profile");
            content = getProfile("name=" + UrlEncode(profile) + "&token=" + access_token + "&expand=true", dummy_str, &ret_code, headers);
        }
        else
        {
            if(ini.GetBool("direct") == true)
            {
                std::string url = ini.Get("url");
                content = fetchFile(url, proxy, cache_subscription);
                if(content.empty())
                {
                    //std::cerr<<"Artifact '"<<x<<"' generate ERROR! Please check your link.\n\n";
                    writeLog(0, "Artifact '" + x + "' generate ERROR! Please check your link.\n", LOG_LEVEL_ERROR);
                    if(sections.size() == 1)
                        return -1;
                }
                // add UTF-8 BOM
                fileWrite(path, "\xEF\xBB\xBF" + content, true);
                continue;
            }
            ini.GetItems(allItems);
            allItems.emplace("expand", "true");
            for(auto &y : allItems)
            {
                if(y.first == "path")
                    continue;
                arguments += y.first + "=" + UrlEncode(y.second) + "&";
            }
            arguments.erase(arguments.size() - 1);
            content = subconverter(arguments, dummy_str, &ret_code, headers);
        }
        if(ret_code != 200)
        {
            //std::cerr<<"Artifact '"<<x<<"' generate ERROR! Reason: "<<content<<"\n\n";
            writeLog(0, "Artifact '" + x + "' generate ERROR! Reason: " + content + "\n", LOG_LEVEL_ERROR);
            if(sections.size() == 1)
                return -1;
            continue;
        }
        fileWrite(path, content, true);
        for(auto &y : headers)
        {
            if(y.first == "Subscription-UserInfo")
            {
                std::cerr<<"User Info for artifact '"<<x<<"': "<<subInfoToMessage(y.second)<<"\n";
                break;
            }
        }
        //std::cerr<<"Artifact '"<<x<<"' generate SUCCESS!\n\n";
        writeLog(0, "Artifact '" + x + "' generate SUCCESS!\n", LOG_LEVEL_INFO);
        eraseElements(headers);
    }
    //std::cerr<<"All artifact generated. Exiting...\n";
    writeLog(0, "All artifact generated. Exiting...", LOG_LEVEL_INFO);
    return 0;
}

std::string renderTemplate(RESPONSE_CALLBACK_ARGS)
{
    std::string path = UrlDecode(getUrlArg(argument, "path"));
    writeLog(0, "Trying to render template '" + path + "'...", LOG_LEVEL_INFO);

    if(path.find(template_path) != 0)
    {
        *status_code = 403;
        return "Out of scope";
    }
    if(!fileExist(path))
    {
        *status_code = 404;
        return "Not found";
    }
    std::string template_content = fetchFile(path, parseProxy(proxy_config), cache_config);
    if(template_content.empty())
    {
        *status_code = 400;
        return "File empty or out of scope";
    }
    template_args tpl_args;
    tpl_args.global_vars = global_vars;

    //load request arguments as template variables
    string_array req_args = split(argument, "&");
    string_size pos;
    string_map req_arg_map;
    for(std::string &x : req_args)
    {
        pos = x.find("=");
        if(pos == x.npos)
            continue;
        req_arg_map[x.substr(0, pos)] = x.substr(pos + 1);
    }
    tpl_args.request_params = req_arg_map;

    std::string output_content;
    if(render_template(template_content, tpl_args, output_content, template_path) != 0)
    {
        *status_code = 400;
        writeLog(0, "Render failed with error.", LOG_LEVEL_WARNING);
    }
    else
        writeLog(0, "Render completed.", LOG_LEVEL_INFO);

    return output_content;
}
