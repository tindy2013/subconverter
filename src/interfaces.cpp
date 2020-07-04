#include <iostream>
#include <string>
#include <mutex>
#include <numeric>

#include <inja.hpp>
#include <yaml-cpp/yaml.h>

#include "yamlcpp_extra.h"
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
#include "script_duktape.h"

//common settings
std::string pref_path = "pref.ini", def_ext_config;
string_array def_exclude_remarks, def_include_remarks, rulesets, stream_rules, time_rules;
std::vector<ruleset_content> ruleset_content_array;
std::string listen_address = "127.0.0.1", default_url, insert_url, managed_config_prefix;
int listen_port = 25500, max_pending_connections = 10, max_concurrent_threads = 4;
bool prepend_insert_url = true;
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
tribool udp_flag, tfo_flag, scv_flag, enable_insert;
bool do_sort = false, config_update_strict = false;
bool clash_use_new_field_name = false;
std::string proxy_config, proxy_ruleset, proxy_subscription;
int config_update_interval = 0;
std::string sort_script, filter_script;

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

//limits
size_t max_allowed_rulesets = 64, max_allowed_rules = 32768;

string_array regex_blacklist = {"(.*)*"};

void refreshRulesets(string_array &ruleset_list, std::vector<ruleset_content> &rca);

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
const string_array clash_rule_type = {basic_types, "IP-CIDR6", "SRC-PORT", "DST-PORT"};
const string_array surge_rule_type = {basic_types, "IP-CIDR6", "USER-AGENT", "URL-REGEX", "AND", "OR", "NOT", "PROCESS-NAME", "IN-PORT", "DEST-PORT", "SRC-IP"};
const string_array quanx_rule_type = {basic_types, "USER-AGENT", "HOST", "HOST-SUFFIX", "HOST-KEYWORD"};

std::string convertRuleset(const std::string &content, int type)
{
    /// Target: Surge type,pattern[,flag]
    /// Source: QuanX type,pattern[,group]
    ///         Clash payload:\n  - 'ipcidr/domain/classic(Surge-like)'

    std::string output, strLine;

    if(type == RULESET_SURGE)
        return content;

    if(startsWith(content, "payload:")) /// Clash
    {
        output = regReplace(regReplace(content, "payload:\\r?\\n", "", true), "\\s?^\\s*-\\s+('?)(.*)\\1$", "\n$2", true);
        if(type == RULESET_CLASH_CLASSICAL) /// classical type
            return output;
        std::stringstream ss;
        ss << output;
        char delimiter = getLineBreak(output);
        output.clear();
        string_size pos, lineSize;
        while(getline(ss, strLine, delimiter))
        {
            strLine = trim(strLine);
            lineSize = strLine.size();
            if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
            {
                strLine.erase(lineSize - 1);
                lineSize--;
            }

            if(!strLine.empty() && (strLine[0] != ';' && strLine[0] != '#' && !(lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')))
            {
                pos = strLine.find("/");
                if(pos != strLine.npos) /// ipcidr
                {
                    if(isIPv4(strLine.substr(0, pos)))
                        output += "IP-CIDR,";
                    else
                        output += "IP-CIDR6,";
                }
                else
                {
                    if(strLine[0] == '.' || (lineSize >= 2 && strLine[0] == '+' && strLine[1] == '.')) /// suffix
                    {
                        bool keyword_flag = false;
                        while(endsWith(strLine, ".*"))
                        {
                            keyword_flag = true;
                            strLine.erase(strLine.size() - 2);
                        }
                        output += "DOMAIN-";
                        if(keyword_flag)
                            output += "KEYWORD,";
                        else
                            output += "SUFFIX,";
                        strLine.erase(0, 2 - (strLine[0] == '.'));
                    }
                    else
                        output += "DOMAIN,";
                }
            }
            output += strLine;
            output += '\n';
        }
        return output;
    }
    else /// QuanX
    {
        output = regReplace(regReplace(content, "^(?i:host)", "DOMAIN", true), "^(?i:ip6-cidr)", "IP-CIDR6", true); //translate type
        output = regReplace(output, "^((?i:DOMAIN(?:-(?:SUFFIX|KEYWORD))?|IP-CIDR6?|USER-AGENT),)\\s*?(\\S*?)(?:,(?!no-resolve).*?)(,no-resolve)?$", "\\U$1\\E$2${3:-}", true); //remove group
        return output;
    }
}

std::string getConvertedRuleset(RESPONSE_CALLBACK_ARGS)
{
    std::string url = UrlDecode(getUrlArg(request.argument, "url")), type = getUrlArg(request.argument, "type");
    return convertRuleset(fetchFile(url, parseProxy(proxy_ruleset), cache_ruleset), to_int(type));
}

std::string getRuleset(RESPONSE_CALLBACK_ARGS)
{
    std::string &argument = request.argument;
    int *status_code = &response.status_code;
    /// type: 1 for Surge, 2 for Quantumult X, 3 for Clash domain rule-provider, 4 for Clash ipcidr rule-provider, 5 for Surge DOMAIN-SET, 6 for Clash classical ruleset
    std::string url = urlsafe_base64_decode(getUrlArg(argument, "url")), type = getUrlArg(argument, "type"), group = urlsafe_base64_decode(getUrlArg(argument, "group"));
    std::string output_content, dummy;
    int type_int = to_int(type, 0);

    if(!url.size() || !type.size() || (type_int == 2 && !group.size()) || (type_int < 1 || type_int > 6))
    {
        *status_code = 400;
        return "Invalid request!";
    }

    std::string proxy = parseProxy(proxy_ruleset);
    string_array vArray = split(url, "|");
    for(std::string &x : vArray)
        x.insert(0, "ruleset,");
    std::vector<ruleset_content> rca;
    refreshRulesets(vArray, rca);
    for(ruleset_content &x : rca)
    {
        std::string content = x.rule_content.get();
        output_content += convertRuleset(content, x.rule_type);
    }

    if(!output_content.size())
    {
        *status_code = 400;
        return "Invalid request!";
    }

    std::string strLine;
    std::stringstream ss;
    const std::string rule_match_regex = "^(.*?,.*?)(,.*)(,.*)$";

    ss << output_content;
    char delimiter = getLineBreak(output_content);
    std::string::size_type lineSize, posb, pose;
    auto filterLine = [&]()
    {
        posb = 0;
        pose = strLine.find(',');
        if(pose == strLine.npos)
            return 1;
        posb = pose + 1;
        pose = strLine.find(',', posb);
        if(pose == strLine.npos)
        {
            pose = strLine.size();
            if(strLine[pose - 1] == '\r')
                pose--;
        }
        else
            pose -= posb;
        return 0;
    };

    lineSize = output_content.size();
    output_content.clear();
    output_content.reserve(lineSize);

    if(type_int == 3 || type_int == 4 || type_int == 6)
        output_content = "payload:\n";

    while(getline(ss, strLine, delimiter))
    {
        switch(type_int)
        {
        case 2:
            if(!std::any_of(quanx_rule_type.begin(), quanx_rule_type.end(), [&strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            break;
        case 1:
            if(!std::any_of(surge_rule_type.begin(), surge_rule_type.end(), [&strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            break;
        case 3:
            if(!startsWith(strLine, "DOMAIN-SUFFIX,") && !startsWith(strLine, "DOMAIN,"))
                continue;
            if(filterLine())
                continue;
            output_content += "  - '";
            if(strLine[posb - 2] == 'X')
                output_content += "+.";
            output_content += strLine.substr(posb, pose);
            output_content += "'\n";
            continue;
        case 4:
            if(!startsWith(strLine, "IP-CIDR,") && !startsWith(strLine, "IP-CIDR6,"))
                continue;
            if(filterLine())
                continue;
            output_content += "  - '";
            output_content += strLine.substr(posb, pose);
            output_content += "'\n";
            continue;
        case 5:
            if(!startsWith(strLine, "DOMAIN-SUFFIX,") && !startsWith(strLine, "DOMAIN,"))
                continue;
            if(filterLine())
                continue;
            output_content += strLine.substr(posb, pose);
            output_content += '\n';
            continue;
        case 6:
            if(!std::any_of(clash_rule_type.begin(), clash_rule_type.end(), [&strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            output_content += "  - ";
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
                if(startsWith(strLine, "IP-CIDR6"))
                    strLine.replace(0, 8, "IP6-CIDR");
                strLine += "," + group;
                if(std::count(strLine.begin(), strLine.end(), ',') > 2 && regReplace(strLine, rule_match_regex, "$2") == ",no-resolve")
                    strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                else
                    strLine = regReplace(strLine, rule_match_regex, "$1$3");
            }
        }
        output_content += strLine;
        output_content += '\n';
    }

    if(output_content == "payload:\n")
    {
        switch(type_int)
        {
        case 3:
            output_content += "  - '--placeholder--'";
            break;
        case 4:
            output_content += "  - '0.0.0.0/32'";
            break;
        case 6:
            output_content += "  - 'DOMAIN,--placeholder--'";
            break;
        }
    }
    return output_content;
}

int importItems(string_array &target, bool scope_limit = true)
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

        std::string proxy = parseProxy(proxy_config);

        if(fileExist(path))
            content = fileGet(path, scope_limit);
        else if(isLink(path))
            content = webGet(path, proxy, cache_config);
        else
            writeLog(0, "File not found or not a valid URL: " + path, LOG_LEVEL_ERROR);
        if(!content.size())
            return -1;

        ss << content;
        char delimiter = getLineBreak(content);
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

void readRegexMatch(YAML::Node node, const std::string &delimiter, string_array &dest, bool scope_limit = true)
{
    YAML::Node object;
    std::string script, url, match, rep, strLine;

    for(unsigned i = 0; i < node.size(); i++)
    {
        object = node[i];
        object["script"] >>= script;
        if(script.size())
        {
            dest.emplace_back("!!script:" + script);
            continue;
        }
        object["import"] >>= url;
        if(url.size())
        {
            dest.emplace_back("!!import:" + url);
            continue;
        }
        object["match"] >>= match;
        object["replace"] >>= rep;
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
        object["import"] >>= url;
        if(url.size())
        {
            url = "!!import:" + url;
            dest.emplace_back(url);
            continue;
        }
        object["match"] >>= match;
        object["emoji"] >>= rep;
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
    std::string strLine, name, type;
    string_array tempArray;
    YAML::Node object;
    unsigned int i, j;

    for(i = 0; i < node.size(); i++)
    {
        eraseElements(tempArray);
        object = node[i];
        object["import"] >>= name;
        if(name.size())
        {
            dest.emplace_back("!!import:" + name);
            continue;
        }
        std::string url = "http://www.gstatic.com/generate_204", interval = "300", tolerance, timeout;
        object["name"] >>= name;
        object["type"] >>= type;
        tempArray.emplace_back(name);
        tempArray.emplace_back(type);
        object["url"] >>= url;
        object["interval"] >>= interval;
        object["tolerance"] >>= tolerance;
        object["timeout"] >>= timeout;
        for(j = 0; j < object["rule"].size(); j++)
            tempArray.emplace_back(safe_as<std::string>(object["rule"][j]));
        switch(hash_(type))
        {
        case "select"_hash:
            if(tempArray.size() < 3)
                continue;
            break;
        case "ssid"_hash:
            if(tempArray.size() < 4)
                continue;
            break;
        default:
            if(tempArray.size() < 3)
                continue;
            tempArray.emplace_back(url);
            tempArray.emplace_back(interval + "," + timeout + "," + tolerance);
        }

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
        object = node[i];
        object["import"] >>= name;
        if(name.size())
        {
            dest.emplace_back("!!import:" + name);
            continue;
        }
        object["ruleset"] >>= url;
        object["group"] >>= group;
        object["rule"] >>= name;
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
    std::string rule_group, rule_url, rule_url_typed;
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
            rc = {rule_group, "", "", RULESET_SURGE, std::async(std::launch::async, [rule_url](){return rule_url;})};
            rca.emplace_back(rc);
            continue;
        }
        else
        {
            ruleset_type type = RULESET_SURGE;
            const std::map<std::string, ruleset_type> types = {{"clash-domain:", RULESET_CLASH_DOMAIN}, {"clash-ipcidr:", RULESET_CLASH_IPCIDR}, {"clash-classic:", RULESET_CLASH_CLASSICAL}, \
            {"quanx:", RULESET_QUANX}, {"surge:", RULESET_SURGE}};
            rule_url_typed = rule_url;
            auto iter = std::find_if(types.begin(), types.end(), [rule_url](auto y){ return startsWith(rule_url, y.first); });
            if(iter != types.end())
            {
                rule_url.erase(0, iter->first.size());
                type = iter->second;
            }
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
            rc = {rule_group, rule_url, rule_url_typed, type, fetchFileAsync(rule_url, proxy, cache_ruleset, async_fetch_ruleset)};
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
    enable_insert = safe_as<std::string>(section["enable_insert"]);
    if(section["insert_url"].IsSequence())
    {
        section["insert_url"] >> tempArray;
        if(tempArray.size())
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(a) + "|" + std::move(b);
            });
            insert_url = strLine;
            eraseElements(tempArray);
        }
    }
    section["prepend_insert_url"] >> prepend_insert_url;
    if(section["exclude_remarks"].IsSequence())
        section["exclude_remarks"] >> def_exclude_remarks;
    if(section["include_remarks"].IsSequence())
        section["include_remarks"] >> def_include_remarks;
    filter_script = safe_as<bool>(section["enable_filter"]) ? safe_as<std::string>(section["filter_script"]) : "";
    if(startsWith(filter_script, "path:"))
        filter_script = fileGet(filter_script.substr(5), false);
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
        /*
        section["udp_flag"] >> udp_flag;
        section["tcp_fast_open_flag"] >> tfo_flag;
        section["skip_cert_verify_flag"] >> scv_flag;
        */
        udp_flag.set(safe_as<std::string>(section["udp_flag"]));
        tfo_flag.set(safe_as<std::string>(section["tcp_fast_open_flag"]));
        scv_flag.set(safe_as<std::string>(section["skip_cert_verify_flag"]));
        section["sort_flag"] >> do_sort;
        section["sort_script"] >> sort_script;
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

    const char *rulesets_title = node["rulesets"].IsDefined() ? "rulesets" : "ruleset";
    if(node[rulesets_title].IsDefined())
    {
        section = node[rulesets_title];
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
        const char *ruleset_title = section["rulesets"].IsDefined() ? "rulesets" : "surge_ruleset";
        if(section[ruleset_title].IsSequence())
            readRuleset(section[ruleset_title], rulesets, false);
    }

    const char *groups_title = node["proxy_groups"].IsDefined() ? "proxy_groups" : "proxy_group";
    if(node[groups_title].IsDefined() && node[groups_title]["custom_proxy_group"].IsDefined())
        readGroup(node[groups_title]["custom_proxy_group"], clash_extra_group, false);

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
        node["advanced"]["max_allowed_rulesets"] >> max_allowed_rulesets;
        node["advanced"]["max_allowed_rules"] >> max_allowed_rules;
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
    enable_insert = ini.Get("enable_insert");
    ini.GetIfExist("insert_url", insert_url);
    ini.GetBoolIfExist("prepend_insert_url", prepend_insert_url);
    if(ini.ItemPrefixExist("exclude_remarks"))
        ini.GetAll("exclude_remarks", def_exclude_remarks);
    if(ini.ItemPrefixExist("include_remarks"))
        ini.GetAll("include_remarks", def_include_remarks);
    filter_script = ini.GetBool("enable_filter") ? ini.Get("filter_script"): "";
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
        /*
        ini.GetBoolIfExist("udp_flag", udp_flag);
        ini.GetBoolIfExist("tcp_fast_open_flag", tfo_flag);
        ini.GetBoolIfExist("skip_cert_verify_flag", scv_flag);
        */
        udp_flag.set(ini.Get("udp_flag"));
        tfo_flag.set(ini.Get("tcp_fast_open_flag"));
        scv_flag.set(ini.Get("skip_cert_verify_flag"));
        ini.GetBoolIfExist("sort_flag", do_sort);
        sort_script = ini.Get("sort_script");
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

    if(ini.SectionExist("rulesets"))
        ini.EnterSection("rulesets");
    else
        ini.EnterSection("ruleset");
    enable_rule_generator = ini.GetBool("enabled");
    if(enable_rule_generator)
    {
        ini.GetBoolIfExist("overwrite_original_rules", overwrite_original_rules);
        ini.GetBoolIfExist("update_ruleset_on_request", update_ruleset_on_request);
        if(ini.ItemPrefixExist("ruleset"))
        {
            ini.GetAll("ruleset", rulesets);
            importItems(rulesets, true);
        }
        else if(ini.ItemPrefixExist("surge_ruleset"))
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

    if(ini.SectionExist("proxy_groups"))
        ini.EnterSection("proxy_groups");
    else
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
    ini.GetNumberIfExist("max_allowed_rulesets", max_allowed_rulesets);
    ini.GetNumberIfExist("max_allowed_rules", max_allowed_rules);
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
    tribool add_emoji;
    tribool remove_old_emoji;
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

    const char *group_name = section["proxy_groups"].IsDefined() ? "proxy_groups" : "custom_proxy_group";
    if(section[group_name].size())
        readGroup(section[group_name], ext.custom_proxy_group, api_mode);

    const char *ruleset_name = section["rulesets"].IsDefined() ? "rulesets" : "surge_ruleset";
    if(section[ruleset_name].size())
    {
        readRuleset(section[ruleset_name], ext.surge_ruleset, api_mode);
        if(max_allowed_rulesets && ext.surge_ruleset.size() > max_allowed_rulesets)
        {
            writeLog(0, "Ruleset count in external config has exceeded limit.", LOG_LEVEL_WARNING);
            eraseElements(ext.surge_ruleset);
            return -1;
        }
    }

    if(section["rename_node"].size())
        readRegexMatch(section["rename_node"], "@", ext.rename, api_mode);

    ext.add_emoji = safe_as<std::string>(section["add_emoji"]);
    ext.remove_old_emoji = safe_as<std::string>(section["remove_old_emoji"]);
    const char *emoji_name = section["emojis"].IsDefined() ? "emojis" : "emoji";
    if(section[emoji_name].size())
        readEmoji(section[emoji_name], ext.emoji, api_mode);

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
    std::string base_content, proxy = parseProxy(proxy_config), config = fetchFile(path, proxy, cache_config);
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
    std::string ruleset_name = ini.ItemPrefixExist("ruleset") ? "ruleset" : "surge_ruleset";
    if(ini.ItemPrefixExist(ruleset_name))
    {
        ini.GetAll(ruleset_name, ext.surge_ruleset);
        importItems(ext.surge_ruleset, api_mode);
        if(max_allowed_rulesets && ext.surge_ruleset.size() > max_allowed_rulesets)
        {
            writeLog(0, "Ruleset count in external config has exceeded limit. ", LOG_LEVEL_WARNING);
            eraseElements(ext.surge_ruleset);
            return -1;
        }
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
    ext.add_emoji = ini.Get("add_emoji");
    ext.remove_old_emoji = ini.Get("remove_old_emoji");
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
    if(isLink(path) || (startsWith(path, base_path) && fileExist(path)))
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
    std::string &argument = request.argument;
    int *status_code = &response.status_code;

    std::string target = getUrlArg(argument, "target");
    switch(hash_(target))
    {
    case "clash"_hash: case "clashr"_hash: case "surge"_hash: case "quan"_hash: case "quanx"_hash: case "loon"_hash: case "surfboard"_hash: case "mellow"_hash: case "ss"_hash: case "ssd"_hash: case "ssr"_hash: case "sssub"_hash: case "v2ray"_hash: case "trojan"_hash: case "mixed"_hash:
        break;
    default:
        *status_code = 400;
        return "Invalid target!";
    }
    //check if we need to read configuration
    if((!api_mode || cfw_child_process) && !generator_mode)
        readConf();

    /// string values
    std::string url = UrlDecode(getUrlArg(argument, "url"));
    std::string group = UrlDecode(getUrlArg(argument, "group")), upload_path = getUrlArg(argument, "upload_path"), version = getUrlArg(argument, "ver");
    std::string include = UrlDecode(getUrlArg(argument, "include")), exclude = UrlDecode(getUrlArg(argument, "exclude"));
    std::string groups = urlsafe_base64_decode(getUrlArg(argument, "groups")), ruleset = urlsafe_base64_decode(getUrlArg(argument, "ruleset")), config = UrlDecode(getUrlArg(argument, "config"));
    std::string dev_id = getUrlArg(argument, "dev_id"), filename = getUrlArg(argument, "filename"), interval_str = getUrlArg(argument, "interval"), strict_str = getUrlArg(argument, "strict");
    std::string ext_rename = UrlDecode(getUrlArg(argument, "rename"));

    /// switches with default value
    tribool upload = getUrlArg(argument, "upload"), emoji = getUrlArg(argument, "emoji"), emoji_add = getUrlArg(argument, "add_emoji"), emoji_remove = getUrlArg(argument, "remove_emoji");
    tribool append_type = getUrlArg(argument, "append_type"), tfo = getUrlArg(argument, "tfo"), udp = getUrlArg(argument, "udp"), nodelist = getUrlArg(argument, "list");
    tribool sort_flag = getUrlArg(argument, "sort"), use_sort_script = getUrlArg(argument, "sort_script");
    tribool clash_new_field = getUrlArg(argument, "new_name"), clash_script = getUrlArg(argument, "script"), add_insert = getUrlArg(argument, "insert");
    tribool scv = getUrlArg(argument, "scv"), fdn = getUrlArg(argument, "fdn"), expand = getUrlArg(argument, "expand"), append_sub_userinfo = getUrlArg(argument, "append_info");
    tribool prepend_insert = getUrlArg(argument, "prepend"), classical = getUrlArg(argument, "classic");

    std::string base_content, output_content;
    string_array extra_group, extra_ruleset, include_remarks = def_include_remarks, exclude_remarks = def_exclude_remarks;
    std::vector<ruleset_content> rca;
    extra_settings ext;
    std::string subInfo, dummy;
    int interval = interval_str.size() ? to_int(interval_str, config_update_interval) : config_update_interval;
    bool ruleset_updated = false, authorized = !api_mode || getUrlArg(argument, "token") == access_token, strict = strict_str.size() ? strict_str == "true" : config_update_strict;

    if(std::find(regex_blacklist.cbegin(), regex_blacklist.cend(), include) != regex_blacklist.cend() || std::find(regex_blacklist.cbegin(), regex_blacklist.cend(), exclude) != regex_blacklist.cend())
        return "Invalid request!";

    //for external configuration
    std::string ext_clash_base = clash_rule_base, ext_surge_base = surge_rule_base, ext_mellow_base = mellow_rule_base, ext_surfboard_base = surfboard_rule_base;
    std::string ext_quan_base = quan_rule_base, ext_quanx_base = quanx_rule_base, ext_loon_base = loon_rule_base, ext_sssub_base = sssub_rule_base;

    //validate urls
    add_insert.define(enable_insert);
    if(!url.size() && (!api_mode || authorized))
        url = default_url;
    if((!url.size() && !(insert_url.size() && add_insert)) || !target.size())
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
        {
            req_arg_map[x] = "";
            continue;
        }
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

    //check other flags
    ext.append_proxy_type = append_type.get(append_proxy_type);
    if((target == "clash" || target == "clashr") && clash_script.is_undef())
        expand.define(true);

    /// read preference from argument, assign global var if not in argument
    ext.tfo.read(tfo).read(tfo_flag);
    ext.udp.read(udp).read(udp_flag);
    ext.skip_cert_verify.read(scv).read(scv_flag);

    ext.sort_flag = sort_flag.get(do_sort);
    use_sort_script.define(sort_script.size() != 0);
    if(ext.sort_flag && use_sort_script)
        ext.sort_script = sort_script;
    ext.filter_deprecated = fdn.get(filter_deprecated);
    ext.clash_new_field_name = clash_new_field.get(clash_use_new_field_name);
    ext.clash_script = clash_script.get();
    ext.clash_classical_ruleset = classical.get();
    if(!expand)
        ext.clash_new_field_name = true;
    else
        ext.clash_script = false;

    ext.nodelist = nodelist;
    ext.surge_ssr_path = surge_ssr_path;
    ext.quanx_dev_id = dev_id.size() ? dev_id : quanx_script_id;
    ext.enable_rule_generator = enable_rule_generator;
    ext.overwrite_original_rules = overwrite_original_rules;
    if(!expand)
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
        if(loadExternalConfig(config, extconf) == 0)
        {
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
            if(extconf.include.size())
                include_remarks = extconf.include;
            if(extconf.exclude.size())
                exclude_remarks = extconf.exclude;
            emoji_add.define(extconf.add_emoji);
            emoji_remove.define(extconf.remove_old_emoji);
        }
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

    if(!emoji.is_undef())
    {
        emoji_add.set(emoji);
        emoji_remove.set(true);
    }
    ext.add_emoji = emoji_add.get(add_emoji);
    ext.remove_emoji = emoji_remove.get(remove_old_emoji);
    if(ext.add_emoji && ext.emoji_array.empty())
        ext.emoji_array = safe_get_emojis();
    if(ext_rename.size())
        ext.rename_array = split(ext_rename, "`");
    else if(ext.rename_array.empty())
        ext.rename_array = safe_get_renames();

    //check custom include/exclude settings
    if(include.size() && regValid(include))
        include_remarks = string_array{include};
    if(exclude.size() && regValid(exclude))
        exclude_remarks = string_array{exclude};

    //start parsing urls
    string_array stream_temp = safe_get_streams(), time_temp = safe_get_times();

    //loading urls
    string_array urls;
    std::vector<nodeInfo> nodes, insert_nodes;
    int groupID = 0;
    if(insert_url.size() && add_insert)
    {
        groupID = -1;
        urls = split(insert_url, "|");
        importItems(urls, true);
        for(std::string &x : urls)
        {
            x = regTrim(x);
            writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
            if(addNodes(x, insert_nodes, groupID, proxy, exclude_remarks, include_remarks, stream_temp, time_temp, subInfo, authorized, request.headers) == -1)
            {
                *status_code = 400;
                return std::string("The following link doesn't contain any valid node info: " + x);
            }
            groupID--;
        }
    }
    urls = split(url, "|");
    importItems(urls, true);
    groupID = 0;
    for(std::string &x : urls)
    {
        x = regTrim(x);
        //std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
        if(addNodes(x, nodes, groupID, proxy, exclude_remarks, include_remarks, stream_temp, time_temp, subInfo, authorized, request.headers) == -1)
        {
            *status_code = 400;
            return std::string("The following link doesn't contain any valid node info: " + x);
        }
        groupID++;
    }
    //exit if found nothing
    if(!nodes.size() && !insert_nodes.size())
    {
        *status_code = 400;
        return "No nodes were found!";
    }
    prepend_insert.define(prepend_insert_url);
    for(nodeInfo &x : insert_nodes)
    {
        if(prepend_insert)
            nodes.emplace(nodes.begin(), x);
        else
            nodes.emplace_back(x);
    }
    //run filter script
    if(filter_script.size())
    {
        if(startsWith(filter_script, "path:"))
            filter_script = fileGet(filter_script.substr(5), false);
        duk_context *ctx = duktape_init();
        if(ctx)
        {
            defer(duk_destroy_heap(ctx);)
            if(duktape_peval(ctx, filter_script) == 0)
            {
                auto filter = [&](const nodeInfo &x)
                {
                    duk_get_global_string(ctx, "filter");
                    duktape_push_nodeinfo(ctx, x);
                    duk_pcall(ctx, 1);
                    return !duktape_get_res_bool(ctx);
                };
                nodes.erase(std::remove_if(nodes.begin(), nodes.end(), filter), nodes.end());
            }
            else
            {
                writeLog(0, "Error when trying to parse script:\n" + duktape_get_err_stack(ctx), LOG_LEVEL_ERROR);
                duk_pop(ctx); /// pop err
            }
        }
    }

    //check custom group name
    if(group.size())
        for(nodeInfo &x : nodes)
            x.group = group;

    if(subInfo.size() && append_sub_userinfo.get(append_userinfo))
        response.headers.emplace("Subscription-UserInfo", subInfo);

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

        if(upload)
            uploadGist(target, upload_path, output_content, false);
        break;
    case "surge"_hash:
        surge_ver = version.size() ? to_int(version, 3) : 3;
        writeLog(0, "Generate target: Surge " + std::to_string(surge_ver), LOG_LEVEL_INFO);

        if(ext.nodelist)
        {
            output_content = netchToSurge(nodes, base_content, dummy_ruleset, dummy_group, surge_ver, ext);

            if(upload)
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

            if(upload)
                uploadGist("surge" + version, upload_path, output_content, true);

            if(write_managed_config && managed_config_prefix.size())
                output_content = "#!MANAGED-CONFIG " + managed_url + (interval ? " interval=" + std::to_string(interval) : "") \
                 + " strict=" + std::string(strict ? "true" : "false") + "\n\n" + output_content;
        }
        break;
    case "surfboard"_hash:
        writeLog(0, "Generate target: Surfboard", LOG_LEVEL_INFO);

        if(render_template(fetchFile(ext_surfboard_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        //base_content = fetchFile(ext_surfboard_base, proxy, cache_config);
        output_content = netchToSurge(nodes, base_content, rca, extra_group, -3, ext);
        if(upload)
            uploadGist("surfboard", upload_path, output_content, true);

        if(write_managed_config && managed_config_prefix.size())
            output_content = "#!MANAGED-CONFIG " + managed_url + (interval ? " interval=" + std::to_string(interval) : "") \
                 + " strict=" + std::string(strict ? "true" : "false") + "\n\n" + output_content;
        break;
    case "mellow"_hash:
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

        if(upload)
            uploadGist("mellow", upload_path, output_content, true);
        break;
    case "sssub"_hash:
        writeLog(0, "Generate target: SS Subscription", LOG_LEVEL_INFO);

        if(render_template(fetchFile(ext_sssub_base, proxy, cache_config), tpl_args, base_content, template_path) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        //base_content = fetchFile(ext_sssub_base, proxy, cache_config);
        output_content = netchToSSSub(base_content, nodes, ext);
        if(upload)
            uploadGist("sssub", upload_path, output_content, false);
        break;
    case "ss"_hash:
        writeLog(0, "Generate target: SS", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 1, ext);
        if(upload)
            uploadGist("ss", upload_path, output_content, false);
        break;
    case "ssr"_hash:
        writeLog(0, "Generate target: SSR", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 2, ext);
        if(upload)
            uploadGist("ssr", upload_path, output_content, false);
        break;
    case "v2ray"_hash:
        writeLog(0, "Generate target: v2rayN", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 4, ext);
        if(upload)
            uploadGist("v2ray", upload_path, output_content, false);
        break;
    case "trojan"_hash:
        writeLog(0, "Generate target: Trojan", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 8, ext);
        if(upload)
            uploadGist("trojan", upload_path, output_content, false);
        break;
    case "mixed"_hash:
        writeLog(0, "Generate target: Standard Subscription", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 15, ext);
        if(upload)
            uploadGist("sub", upload_path, output_content, false);
        break;
    case "quan"_hash:
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

        if(upload)
            uploadGist("quan", upload_path, output_content, false);
        break;
    case "quanx"_hash:
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

        if(upload)
            uploadGist("quanx", upload_path, output_content, false);
        break;
    case "loon"_hash:
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

        if(upload)
            uploadGist("loon", upload_path, output_content, false);
        break;
    case "ssd"_hash:
        writeLog(0, "Generate target: SSD", LOG_LEVEL_INFO);
        output_content = netchToSSD(nodes, group, subInfo, ext);
        if(upload)
            uploadGist("ssd", upload_path, output_content, false);
        break;
    default:
        writeLog(0, "Generate target: Unspecified", LOG_LEVEL_INFO);
        *status_code = 500;
        return "Unrecognized target";
    }
    writeLog(0, "Generate completed.", LOG_LEVEL_INFO);
    if(filename.size())
        response.headers.emplace("Content-Disposition", "attachment; filename=\"" + filename + "\"");
    return output_content;
}

std::string simpleToClashR(RESPONSE_CALLBACK_ARGS)
{
    std::string &argument = request.argument;
    int *status_code = &response.status_code;

    std::string url = argument.size() <= 8 ? "" : argument.substr(8);
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
    request.argument = "target=clashr&url=" + UrlEncode(url);
    return subconverter(request, response);
}

std::string surgeConfToClash(RESPONSE_CALLBACK_ARGS)
{
    std::string &argument = request.argument;
    int *status_code = &response.status_code;

    INIReader ini;
    string_array dummy_str_array;
    std::vector<nodeInfo> nodes;
    std::string base_content, url = argument.size() <= 5 ? "" : argument.substr(5);
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
        if(addNodes(x, nodes, 0, proxy, dummy_str_array, dummy_str_array, dummy_str_array, dummy_str_array, subInfo, !api_mode, request.headers) == -1)
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

    extra_settings ext = {true, true, dummy_str_array, dummy_str_array, false, false, false, false, do_sort, filter_deprecated, clash_use_new_field_name, false, "", "", ""};

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
        switch(hash_(content))
        {
        case "direct"_hash:
            singlegroup["name"] = name;
            singlegroup["type"] = "select";
            singlegroup["proxies"].push_back("DIRECT");
            break;
        case "reject"_hash:
        case "reject-tinygif"_hash:
            singlegroup["name"] = name;
            singlegroup["type"] = "select";
            singlegroup["proxies"].push_back("REJECT");
            break;
        default:
            continue;
        }
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
            content = webGet(strArray[1], proxy, cache_ruleset);
            if(!content.size())
                continue;

            ss << content;
            char delimiter = getLineBreak(content);

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
    std::string &argument = request.argument;
    int *status_code = &response.status_code;

    std::string name = UrlDecode(getUrlArg(argument, "name")), token = UrlDecode(getUrlArg(argument, "token"));
    string_array profiles = split(name, "|");
    name = profiles[0];
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
    if(profiles.size() == 1 && profile_token != contents.end())
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
    /// check if more than one profile is provided
    if(profiles.size() > 1)
    {
        writeLog(0, "Multiple profiles are provided. Trying to combine profiles...", LOG_TYPE_INFO);
        std::string all_urls, url;
        auto iter = contents.find("url");
        if(iter != contents.end())
            all_urls = iter->second;
        for(size_t i = 1; i < profiles.size(); i++)
        {
            name = profiles[i];
            if(!fileExist(name))
            {
                writeLog(0, "Ignoring non-exist profile '" + name + "'...", LOG_LEVEL_WARNING);
                continue;
            }
            if(ini.ParseFile(name) != INIREADER_EXCEPTION_NONE && !ini.SectionExist("Profile"))
            {
                writeLog(0, "Ignoring broken profile '" + name + "'...", LOG_LEVEL_WARNING);
                continue;
            }
            url = ini.Get("Profile", "url");
            if(url.size())
            {
                all_urls += "|" + url;
                writeLog(0, "Profile url from '" + name + "' added.", LOG_LEVEL_INFO);
            }
            else
            {
                writeLog(0, "Profile '" + name + "' does not have url key. Skipping...", LOG_LEVEL_INFO);
            }
        }
        iter->second = all_urls;
    }

    contents.emplace("token", token);
    contents.emplace("profile_data", base64_encode(managed_config_prefix + "/getprofile?" + argument));
    std::string query = std::accumulate(contents.begin(), contents.end(), std::string(), [](const std::string &x, auto y){ return x + y.first + "=" + UrlEncode(y.second) + "&"; });
    query += argument;
    request.argument = query;
    return subconverter(request, response);
}

std::string getScript(RESPONSE_CALLBACK_ARGS)
{
    std::string &argument = request.argument;

    std::string url = urlsafe_base64_decode(getUrlArg(argument, "url")), dev_id = getUrlArg(argument, "id");
    std::string output_content;

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
    std::string &argument = request.argument;

    std::string url = urlsafe_base64_decode(getUrlArg(argument, "url")), dev_id = getUrlArg(argument, "id");
    std::string output_content;

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
        char delimiter = getLineBreak(output_content);

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
    std::string data = args.at(0)->get<std::string>(), hostname;
    const std::string matcher = R"(^(?i:hostname\s*?=\s*?)(.*?)\s$)";
    string_array urls = split(data, ",");
    if(!urls.size())
        return std::string();

    std::string input_content, output_content, proxy = parseProxy(proxy_config);
    for(std::string &x : urls)
    {
        input_content = webGet(x, proxy, cache_config);
        regGetMatch(input_content, matcher, 2, 0, &hostname);
        if(hostname.size())
        {
            output_content += hostname + ",";
            hostname.clear();
        }
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
    std::string proxy = parseProxy(proxy_subscription);
    Request request;
    Response response;
    int &ret_code = response.status_code;
    string_map &headers = response.headers;
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
            request.argument = "name=" + UrlEncode(profile) + "&token=" + access_token + "&expand=true";
            content = getProfile(request, response);
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
            request.argument = arguments;
            content = subconverter(request, response);
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
        auto iter = std::find_if(headers.begin(), headers.end(), [](auto y){ return y.first == "Subscription-UserInfo"; });
        if(iter != headers.end())
            writeLog(0, "User Info for artifact '" + x + "': " + subInfoToMessage(iter->second), LOG_LEVEL_INFO);
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
    std::string &argument = request.argument;
    int *status_code = &response.status_code;

    std::string path = UrlDecode(getUrlArg(argument, "path"));
    writeLog(0, "Trying to render template '" + path + "'...", LOG_LEVEL_INFO);

    if(path.find(template_path) != 0 || !fileExist(path))
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
            req_arg_map[x] = "";
        else
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
