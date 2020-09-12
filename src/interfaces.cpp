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
std::string gPrefPath = "pref.ini", gDefaultExtConfig;
string_array gExcludeRemarks, gIncludeRemarks, gCustomRulesets, gStreamNodeRules, gTimeNodeRules;
std::vector<ruleset_content> gRulesetContent;
std::string gListenAddress = "127.0.0.1", gDefaultUrls, gInsertUrls, gManagedConfigPrefix;
int gListenPort = 25500, gMaxPendingConns = 10, gMaxConcurThreads = 4;
bool gPrependInsert = true, gSkipFailedLinks = false;
bool gAPIMode = true, gWriteManagedConfig = false, gEnableRuleGen = true, gUpdateRulesetOnRequest = false, gOverwriteOriginalRules = true;
bool gPrintDbgInfo = false, gCFWChildProcess = false, gAppendUserinfo = true, gAsyncFetchRuleset = false, gSurgeResolveHostname = true;
std::string gAccessToken, gBasePath = "base";
extern std::string custom_group;
extern int gLogLevel;
extern long gMaxAllowedDownloadSize;
string_map gAliases;

extern bool gServeFile;
extern std::string gServeFileRoot;

//global variables for template
std::string gTemplatePath = "templates";
string_map gTemplateVars;

//generator settings
bool gGeneratorMode = false;
std::string gGenerateProfiles;

//multi-thread lock
std::mutex gMutexConfigure;

//preferences
string_array gRenames, gEmojis;
bool gAddEmoji = false, gRemoveEmoji = false, gAppendType = false, gFilterDeprecated = true;
tribool gUDP, gTFO, gSkipCertVerify, gTLS13, gEnableInsert;
bool gEnableSort = false, gUpdateStrict = false;
bool gClashUseNewField = false;
std::string gClashProxiesStyle = "flow";
std::string gProxyConfig, gProxyRuleset, gProxySubscription;
int gUpdateInterval = 0;
std::string gSortScript, gFilterScript;

std::string gClashBase;
string_array gCustomProxyGroups;
std::string gSurgeBase, gSurfboardBase, gMellowBase, gQuanBase, gQuanXBase, gLoonBase, gSSSubBase;
std::string gSurgeSSRPath, gQuanXDevID;

//cache system
bool gServeCacheOnFetchFail = false;
int gCacheSubscription = 60, gCacheConfig = 300, gCacheRuleset = 21600;

//limits
size_t gMaxAllowedRulesets = 64, gMaxAllowedRules = 32768;

string_array gRegexBlacklist = {"(.*)*"};

void refreshRulesets(string_array &ruleset_list, std::vector<ruleset_content> &ruleset_content_array);

std::string parseProxy(const std::string &source)
{
    std::string proxy = source;
    if(source == "SYSTEM")
        proxy = getSystemProxy();
    else if(source == "NONE")
        proxy = "";
    return proxy;
}

extern string_array ClashRuleTypes, SurgeRuleTypes, QuanXRuleTypes;
const std::map<std::string, ruleset_type> RulesetTypes = {{"clash-domain:", RULESET_CLASH_DOMAIN}, {"clash-ipcidr:", RULESET_CLASH_IPCIDR}, {"clash-classic:", RULESET_CLASH_CLASSICAL}, \
            {"quanx:", RULESET_QUANX}, {"surge:", RULESET_SURGE}};

struct UAProfile
{
    std::string head;
    std::string version_match;
    std::string version_target;
    std::string target;
    tribool clash_new_name = tribool();
    int surge_ver = -1;
};

const std::vector<UAProfile> UAMatchList = {
    {"ClashForAndroid","\\/([0-9.]+)","2.0","clash",true},
    {"ClashForAndroid","\\/([0-9.]+)R","","clashr",false},
    {"ClashForAndroid","","","clash",false},
    {"ClashforWindows","\\/([0-9.]+)","0.11","clash",true},
    {"ClashforWindows","","","clash",false},
    {"ClashX Pro","","","clash",true},
    {"ClashX","\\/([0-9.]+)","0.13","clash",true},
    {"Clash","","","clash",true},
    {"Kitsunebi","","","v2ray"},
    {"Loon","","","loon"},
    {"Pharos","","","mixed"},
    {"Potatso","","","mixed"},
    {"Quantumult%20X","","","quanx"},
    {"Quantumult","","","quan"},
    {"Qv2ray","","","v2ray"},
    {"Shadowrocket","","","mixed"},
    {"Surfboard","","","surfboard"},
    {"Surge","\\/([0-9.]+).*x86","906","surge",false,4}, /// Surge for Mac (supports VMess)
    {"Surge","\\/([0-9.]+).*x86","368","surge",false,3}, /// Surge for Mac (supports new rule types and Shadowsocks without plugin)
    {"Surge","\\/([0-9.]+)","1419","surge",false,4}, /// Surge iOS 4 (first version)
    {"Surge","\\/([0-9.]+)","900","surge",false,3}, /// Surge iOS 3 (approx)
    {"Surge","","","surge",false,2}, /// any version of Surge as fallback
    {"Trojan-Qt5","","","trojan"},
    {"V2rayU","","","v2ray"},
    {"V2RayX","","","v2ray"}
};

bool verGreaterEqual(const std::string &src_ver, const std::string &target_ver)
{
    string_size src_pos_beg = 0, src_pos_end, target_pos_beg = 0, target_pos_end;
    while(true)
    {
        src_pos_end = src_ver.find('.', src_pos_beg);
        if(src_pos_end == src_ver.npos)
            src_pos_end = src_ver.size();
        int part_src = std::stoi(src_ver.substr(src_pos_beg, src_pos_end - src_pos_beg));
        target_pos_end = target_ver.find('.', target_pos_beg);
        if(target_pos_end == target_ver.npos)
            target_pos_end = target_ver.size();
        int part_target = std::stoi(target_ver.substr(target_pos_beg, target_pos_end - target_pos_beg));
        if(part_src > part_target)
            break;
        else if(part_src < part_target)
            return false;
        else if(src_pos_end >= src_ver.size() - 1 || target_pos_end >= target_ver.size() - 1)
            break;
        src_pos_beg = src_pos_end + 1;
        target_pos_beg = target_pos_end + 1;
    }
    return true;

}

void matchUserAgent(const std::string &user_agent, std::string &target, tribool &clash_new_name, int &surge_ver)
{
    if(user_agent.empty())
        return;
    for(const UAProfile &x : UAMatchList)
    {
        if(startsWith(user_agent, x.head))
        {
            if(!x.version_match.empty())
            {
                std::string version;
                if(regGetMatch(user_agent, x.version_match, 2, 0, &version))
                    continue;
                if(!x.version_target.empty() && !verGreaterEqual(version, x.version_target))
                    continue;
            }
            target = x.target;
            clash_new_name = x.clash_new_name;
            if(x.surge_ver != -1)
                surge_ver = x.surge_ver;
            return;
        }
    }
    return;
}

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
                strLine.erase(--lineSize);

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
    return convertRuleset(fetchFile(url, parseProxy(gProxyRuleset), gCacheRuleset), to_int(type));
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

    std::string proxy = parseProxy(gProxyRuleset);
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
            if(!std::any_of(QuanXRuleTypes.begin(), QuanXRuleTypes.end(), [&strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            break;
        case 1:
            if(!std::any_of(SurgeRuleTypes.begin(), SurgeRuleTypes.end(), [&strLine](std::string type){return startsWith(strLine, type);}))
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
            if(!std::any_of(ClashRuleTypes.begin(), ClashRuleTypes.end(), [&strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            output_content += "  - ";
        }

        lineSize = strLine.size();
        if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
            strLine.erase(--lineSize);

        if(!strLine.empty() && (strLine[0] != ';' && strLine[0] != '#' && !(lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')))
        {
            if(type_int == 2)
            {
                if(startsWith(strLine, "IP-CIDR6"))
                    strLine.replace(0, 8, "IP6-CIDR");
                strLine += "," + group;
                if(count_least(strLine, ',', 3) && regReplace(strLine, rule_match_regex, "$2") == ",no-resolve")
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

        std::string proxy = parseProxy(gProxyConfig);

        if(fileExist(path))
            content = fileGet(path, scope_limit);
        else if(isLink(path))
            content = webGet(path, proxy, gCacheConfig);
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
                strLine.erase(--lineSize);
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            result.emplace_back(std::move(strLine));
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
        dest.emplace_back(std::move(strLine));
    }
    importItems(dest, scope_limit);
}

void readEmoji(YAML::Node node, string_array &dest, bool scope_limit = true)
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
        dest.emplace_back(std::move(strLine));
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
        tempArray.emplace_back(std::move(name));
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
            tempArray.emplace_back(std::move(url));
            tempArray.emplace_back(interval + "," + timeout + "," + tolerance);
        }

        strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b) -> std::string
        {
            return std::move(a) + "`" + std::move(b);
        });
        dest.emplace_back(std::move(strLine));
    }
    importItems(dest, scope_limit);
}

void readRuleset(YAML::Node node, string_array &dest, bool scope_limit = true)
{
    std::string strLine, name, url, group, interval;
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
        object["interval"] >>= interval;
        if(url.size())
        {
            strLine = group + "," + url;
            if(interval.size())
                strLine += "," + interval;
        }
        else if(name.size())
            strLine = group + ",[]" + name;
        else
            continue;
        dest.emplace_back(std::move(strLine));
    }
    importItems(dest, scope_limit);
}

void refreshRulesets(string_array &ruleset_list, std::vector<ruleset_content> &ruleset_content_array)
{
    eraseElements(ruleset_content_array);
    std::string rule_group, rule_url, rule_url_typed, interval;
    ruleset_content rc;

    std::string proxy = parseProxy(gProxyRuleset);

    for(std::string &x : ruleset_list)
    {
        string_size pos = x.find(",");
        if(pos == x.npos || pos == x.size() - 1)
            continue;
        rule_group = trim(x.substr(0, pos));
        if(x.find("[]", pos + 1) == pos + 1)
        {
            rule_url = trim(x.substr(pos + 1));
            writeLog(0, "Adding rule '" + rule_url.substr(2) + "," + rule_group + "'.", LOG_LEVEL_INFO);
            rc = {rule_group, "", "", RULESET_SURGE, std::async(std::launch::async, [rule_url](){return rule_url;}), 0};
        }
        else
        {
            string_size pos2 = x.find(",", pos + 1);
            rule_url = trim(x.substr(pos + 1, pos2 - pos - 1));
            if(pos2 != x.npos)
                interval = x.substr(pos2 + 1);
            else
                interval.clear();
            ruleset_type type = RULESET_SURGE;
            rule_url_typed = rule_url;
            auto iter = std::find_if(RulesetTypes.begin(), RulesetTypes.end(), [rule_url](auto y){ return startsWith(rule_url, y.first); });
            if(iter != RulesetTypes.end())
            {
                rule_url.erase(0, iter->first.size());
                type = iter->second;
            }
            writeLog(0, "Updating ruleset url '" + rule_url + "' with group '" + rule_group + "'.", LOG_LEVEL_INFO);
            rc = {rule_group, rule_url, rule_url_typed, type, fetchFileAsync(rule_url, proxy, gCacheRuleset, gAsyncFetchRuleset), to_int(interval, 0)};
        }
        ruleset_content_array.emplace_back(std::move(rc));
    }
    ruleset_content_array.shrink_to_fit();
}

void readYAMLConf(YAML::Node &node)
{
    YAML::Node section = node["common"];
    std::string strLine;
    string_array tempArray;

    section["api_mode"] >> gAPIMode;
    section["api_access_token"] >> gAccessToken;
    if(section["default_url"].IsSequence())
    {
        section["default_url"] >> tempArray;
        if(tempArray.size())
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(a) + "|" + std::move(b);
            });
            gDefaultUrls = strLine;
            eraseElements(tempArray);
        }
    }
    gEnableInsert = safe_as<std::string>(section["enable_insert"]);
    if(section["insert_url"].IsSequence())
    {
        section["insert_url"] >> tempArray;
        if(tempArray.size())
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(a) + "|" + std::move(b);
            });
            gInsertUrls = strLine;
            eraseElements(tempArray);
        }
    }
    section["prepend_insert_url"] >> gPrependInsert;
    if(section["exclude_remarks"].IsSequence())
        section["exclude_remarks"] >> gExcludeRemarks;
    if(section["include_remarks"].IsSequence())
        section["include_remarks"] >> gIncludeRemarks;
    gFilterScript = safe_as<bool>(section["enable_filter"]) ? safe_as<std::string>(section["filter_script"]) : "";
    if(startsWith(gFilterScript, "path:"))
        gFilterScript = fileGet(gFilterScript.substr(5), false);
    section["base_path"] >> gBasePath;
    section["clash_rule_base"] >> gClashBase;
    section["surge_rule_base"] >> gSurgeBase;
    section["surfboard_rule_base"] >> gSurfboardBase;
    section["mellow_rule_base"] >> gMellowBase;
    section["quan_rule_base"] >> gQuanBase;
    section["quanx_rule_base"] >> gQuanXBase;
    section["loon_rule_base"] >> gLoonBase;
    section["sssub_rule_base"] >> gSSSubBase;

    section["default_external_config"] >> gDefaultExtConfig;
    section["append_proxy_type"] >> gAppendType;
    section["proxy_config"] >> gProxyConfig;
    section["proxy_ruleset"] >> gProxyRuleset;
    section["proxy_subscription"] >> gProxySubscription;

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
        gUDP.set(safe_as<std::string>(section["udp_flag"]));
        gTFO.set(safe_as<std::string>(section["tcp_fast_open_flag"]));
        gSkipCertVerify.set(safe_as<std::string>(section["skip_cert_verify_flag"]));
        gTLS13.set(safe_as<std::string>(section["tls13_flag"]));
        section["sort_flag"] >> gEnableSort;
        section["sort_script"] >> gSortScript;
        section["filter_deprecated_nodes"] >> gFilterDeprecated;
        section["append_sub_userinfo"] >> gAppendUserinfo;
        section["clash_use_new_field_name"] >> gClashUseNewField;
        section["clash_proxies_style"] >> gClashProxiesStyle;
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
        section["write_managed_config"] >> gWriteManagedConfig;
        section["managed_config_prefix"] >> gManagedConfigPrefix;
        section["config_update_interval"] >> gUpdateInterval;
        section["config_update_strict"] >> gUpdateStrict;
        section["quanx_device_id"] >> gQuanXDevID;
    }

    if(node["surge_external_proxy"].IsDefined())
    {
        node["surge_external_proxy"]["surge_ssr_path"] >> gSurgeSSRPath;
        node["surge_external_proxy"]["resolve_hostname"] >> gSurgeResolveHostname;
    }

    if(node["emojis"].IsDefined())
    {
        section = node["emojis"];
        section["add_emoji"] >> gAddEmoji;
        section["remove_old_emoji"] >> gRemoveEmoji;
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
        section["enabled"] >> gEnableRuleGen;
        if(!gEnableRuleGen)
        {
            gOverwriteOriginalRules = false;
            gUpdateRulesetOnRequest = false;
        }
        else
        {
            section["overwrite_original_rules"] >> gOverwriteOriginalRules;
            section["update_ruleset_on_request"] >> gUpdateRulesetOnRequest;
        }
        const char *ruleset_title = section["rulesets"].IsDefined() ? "rulesets" : "surge_ruleset";
        if(section[ruleset_title].IsSequence())
            readRuleset(section[ruleset_title], gCustomRulesets, false);
    }

    const char *groups_title = node["proxy_groups"].IsDefined() ? "proxy_groups" : "proxy_group";
    if(node[groups_title].IsDefined() && node[groups_title]["custom_proxy_group"].IsDefined())
        readGroup(node[groups_title]["custom_proxy_group"], gCustomProxyGroups, false);

    if(node["template"].IsDefined())
    {
        node["template"]["template_path"] >> gTemplatePath;
        if(node["template"]["globals"].IsSequence())
        {
            eraseElements(gTemplateVars);
            for(size_t i = 0; i < node["template"]["globals"].size(); i++)
            {
                std::string key, value;
                node["template"]["globals"][i]["key"] >> key;
                node["template"]["globals"][i]["value"] >> value;
                gTemplateVars[key] = value;
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
        node["server"]["listen"] >> gListenAddress;
        node["server"]["port"] >> gListenPort;
        node["server"]["serve_file_root"] >>= gServeFileRoot;
        gServeFile = !gServeFileRoot.empty();
    }

    if(node["advanced"].IsDefined())
    {
        std::string log_level;
        node["advanced"]["log_level"] >> log_level;
        node["advanced"]["print_debug_info"] >> gPrintDbgInfo;
        if(gPrintDbgInfo)
            gLogLevel = LOG_LEVEL_VERBOSE;
        else
        {
            switch(hash_(log_level))
            {
            case "warn"_hash:
                gLogLevel = LOG_LEVEL_WARNING;
                break;
            case "error"_hash:
                gLogLevel = LOG_LEVEL_ERROR;
                break;
            case "fatal"_hash:
                gLogLevel = LOG_LEVEL_FATAL;
                break;
            case "verbose"_hash:
                gLogLevel = LOG_LEVEL_VERBOSE;
                break;
            case "debug"_hash:
                gLogLevel = LOG_LEVEL_DEBUG;
                break;
            default:
                gLogLevel = LOG_LEVEL_INFO;
            }
        }
        node["advanced"]["max_pending_connections"] >> gMaxPendingConns;
        node["advanced"]["max_concurrent_threads"] >> gMaxConcurThreads;
        node["advanced"]["max_allowed_rulesets"] >> gMaxAllowedRulesets;
        node["advanced"]["max_allowed_rules"] >> gMaxAllowedRules;
        node["advanced"]["max_allowed_download_size"] >> gMaxAllowedDownloadSize;
        if(node["advanced"]["enable_cache"].IsDefined())
        {
            if(safe_as<bool>(node["advanced"]["enable_cache"]))
            {
                node["advanced"]["cache_subscription"] >> gCacheSubscription;
                node["advanced"]["cache_config"] >> gCacheConfig;
                node["advanced"]["cache_ruleset"] >> gCacheRuleset;
                node["advanced"]["serve_cache_on_fetch_fail"] >> gServeCacheOnFetchFail;
            }
            else
                gCacheSubscription = gCacheConfig = gCacheRuleset = 0; //disable cache
        }
        node["advanced"]["async_fetch_ruleset"] >> gAsyncFetchRuleset;
        node["advanced"]["skip_failed_links"] >> gSkipFailedLinks;
    }
}

void readConf()
{
    guarded_mutex guard(gMutexConfigure);
    //std::cerr<<"Reading preference settings..."<<std::endl;
    writeLog(0, "Reading preference settings...", LOG_LEVEL_INFO);

    eraseElements(gExcludeRemarks);
    eraseElements(gIncludeRemarks);
    eraseElements(gCustomProxyGroups);
    eraseElements(gCustomRulesets);

    try
    {
        std::string prefdata = fileGet(gPrefPath, false);
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
    int retVal = ini.ParseFile(gPrefPath);
    if(retVal != INIREADER_EXCEPTION_NONE)
    {
        //std::cerr<<"Unable to load preference settings. Reason: "<<ini.GetLastError()<<"\n";
        writeLog(0, "Unable to load preference settings. Reason: " + ini.GetLastError(), LOG_LEVEL_FATAL);
        return;
    }

    string_array tempArray;

    ini.EnterSection("common");
    ini.GetBoolIfExist("api_mode", gAPIMode);
    ini.GetIfExist("api_access_token", gAccessToken);
    ini.GetIfExist("default_url", gDefaultUrls);
    gEnableInsert = ini.Get("enable_insert");
    ini.GetIfExist("insert_url", gInsertUrls);
    ini.GetBoolIfExist("prepend_insert_url", gPrependInsert);
    if(ini.ItemPrefixExist("exclude_remarks"))
        ini.GetAll("exclude_remarks", gExcludeRemarks);
    if(ini.ItemPrefixExist("include_remarks"))
        ini.GetAll("include_remarks", gIncludeRemarks);
    gFilterScript = ini.GetBool("enable_filter") ? ini.Get("filter_script"): "";
    ini.GetIfExist("base_path", gBasePath);
    ini.GetIfExist("clash_rule_base", gClashBase);
    ini.GetIfExist("surge_rule_base", gSurgeBase);
    ini.GetIfExist("surfboard_rule_base", gSurfboardBase);
    ini.GetIfExist("mellow_rule_base", gMellowBase);
    ini.GetIfExist("quan_rule_base", gQuanBase);
    ini.GetIfExist("quanx_rule_base", gQuanXBase);
    ini.GetIfExist("loon_rule_base", gLoonBase);
    ini.GetIfExist("default_external_config", gDefaultExtConfig);
    ini.GetBoolIfExist("append_proxy_type", gAppendType);
    ini.GetIfExist("proxy_config", gProxyConfig);
    ini.GetIfExist("proxy_ruleset", gProxyRuleset);
    ini.GetIfExist("proxy_subscription", gProxySubscription);

    if(ini.SectionExist("surge_external_proxy"))
    {
        ini.EnterSection("surge_external_proxy");
        ini.GetIfExist("surge_ssr_path", gSurgeSSRPath);
        ini.GetBoolIfExist("resolve_hostname", gSurgeResolveHostname);
    }

    if(ini.SectionExist("node_pref"))
    {
        ini.EnterSection("node_pref");
        /*
        ini.GetBoolIfExist("udp_flag", udp_flag);
        ini.GetBoolIfExist("tcp_fast_open_flag", tfo_flag);
        ini.GetBoolIfExist("skip_cert_verify_flag", scv_flag);
        */
        gUDP.set(ini.Get("udp_flag"));
        gTFO.set(ini.Get("tcp_fast_open_flag"));
        gSkipCertVerify.set(ini.Get("skip_cert_verify_flag"));
        gTLS13.set(ini.Get("tls13_flag"));
        ini.GetBoolIfExist("sort_flag", gEnableSort);
        gSortScript = ini.Get("sort_script");
        ini.GetBoolIfExist("filter_deprecated_nodes", gFilterDeprecated);
        ini.GetBoolIfExist("append_sub_userinfo", gAppendUserinfo);
        ini.GetBoolIfExist("clash_use_new_field_name", gClashUseNewField);
        ini.GetIfExist("clash_proxies_style", gClashProxiesStyle);
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
    ini.GetBoolIfExist("write_managed_config", gWriteManagedConfig);
    ini.GetIfExist("managed_config_prefix", gManagedConfigPrefix);
    ini.GetIntIfExist("config_update_interval", gUpdateInterval);
    ini.GetBoolIfExist("config_update_strict", gUpdateStrict);
    ini.GetIfExist("quanx_device_id", gQuanXDevID);

    ini.EnterSection("emojis");
    ini.GetBoolIfExist("add_emoji", gAddEmoji);
    ini.GetBoolIfExist("remove_old_emoji", gRemoveEmoji);
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
    gEnableRuleGen = ini.GetBool("enabled");
    if(gEnableRuleGen)
    {
        ini.GetBoolIfExist("overwrite_original_rules", gOverwriteOriginalRules);
        ini.GetBoolIfExist("update_ruleset_on_request", gUpdateRulesetOnRequest);
        if(ini.ItemPrefixExist("ruleset"))
        {
            ini.GetAll("ruleset", gCustomRulesets);
            importItems(gCustomRulesets, true);
        }
        else if(ini.ItemPrefixExist("surge_ruleset"))
        {
            ini.GetAll("surge_ruleset", gCustomRulesets);
            importItems(gCustomRulesets, false);
        }
    }
    else
    {
        gOverwriteOriginalRules = false;
        gUpdateRulesetOnRequest = false;
    }

    if(ini.SectionExist("proxy_groups"))
        ini.EnterSection("proxy_groups");
    else
        ini.EnterSection("clash_proxy_group");
    if(ini.ItemPrefixExist("custom_proxy_group"))
    {
        ini.GetAll("custom_proxy_group", gCustomProxyGroups);
        importItems(gCustomProxyGroups, false);
    }

    ini.EnterSection("template");
    ini.GetIfExist("template_path", gTemplatePath);
    string_multimap tempmap;
    ini.GetItems(tempmap);
    eraseElements(gTemplateVars);
    for(auto &x : tempmap)
    {
        if(x.first == "template_path")
            continue;
        gTemplateVars[x.first] = x.second;
    }
    gTemplateVars["managed_config_prefix"] = gManagedConfigPrefix;

    if(ini.SectionExist("aliases"))
    {
        ini.EnterSection("aliases");
        ini.GetItems(tempmap);
        reset_redirect();
        for(auto &x : tempmap)
            append_redirect(x.first, x.second);
    }

    ini.EnterSection("server");
    ini.GetIfExist("listen", gListenAddress);
    ini.GetIntIfExist("port", gListenPort);
    gServeFileRoot = ini.Get("serve_file_root");
    gServeFile = !gServeFileRoot.empty();

    ini.EnterSection("advanced");
    std::string log_level;
    ini.GetIfExist("log_level", log_level);
    ini.GetBoolIfExist("print_debug_info", gPrintDbgInfo);
    if(gPrintDbgInfo)
        gLogLevel = LOG_LEVEL_VERBOSE;
    else
    {
        switch(hash_(log_level))
        {
        case "warn"_hash:
            gLogLevel = LOG_LEVEL_WARNING;
            break;
        case "error"_hash:
            gLogLevel = LOG_LEVEL_ERROR;
            break;
        case "fatal"_hash:
            gLogLevel = LOG_LEVEL_FATAL;
            break;
        case "verbose"_hash:
            gLogLevel = LOG_LEVEL_VERBOSE;
            break;
        case "debug"_hash:
            gLogLevel = LOG_LEVEL_DEBUG;
            break;
        default:
            gLogLevel = LOG_LEVEL_INFO;
        }
    }
    ini.GetIntIfExist("max_pending_connections", gMaxPendingConns);
    ini.GetIntIfExist("max_concurrent_threads", gMaxConcurThreads);
    ini.GetNumberIfExist("max_allowed_rulesets", gMaxAllowedRulesets);
    ini.GetNumberIfExist("max_allowed_rules", gMaxAllowedRules);
    ini.GetNumberIfExist("max_allowed_download_size", gMaxAllowedDownloadSize);
    if(ini.ItemExist("enable_cache"))
    {
        if(ini.GetBool("enable_cache"))
        {
            ini.GetIntIfExist("cache_subscription", gCacheSubscription);
            ini.GetIntIfExist("cache_config", gCacheConfig);
            ini.GetIntIfExist("cache_ruleset", gCacheRuleset);
            ini.GetBoolIfExist("serve_cache_on_fetch_fail", gServeCacheOnFetchFail);
        }
        else
        {
            gCacheSubscription = gCacheConfig = gCacheRuleset = 0; //disable cache
            gServeCacheOnFetchFail = false;
        }
    }
    ini.GetBoolIfExist("async_fetch_ruleset", gAsyncFetchRuleset);
    ini.GetBoolIfExist("skip_failed_links", gSkipFailedLinks);

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
        readGroup(section[group_name], ext.custom_proxy_group, gAPIMode);

    const char *ruleset_name = section["rulesets"].IsDefined() ? "rulesets" : "surge_ruleset";
    if(section[ruleset_name].size())
    {
        readRuleset(section[ruleset_name], ext.surge_ruleset, gAPIMode);
        if(gMaxAllowedRulesets && ext.surge_ruleset.size() > gMaxAllowedRulesets)
        {
            writeLog(0, "Ruleset count in external config has exceeded limit.", LOG_LEVEL_WARNING);
            eraseElements(ext.surge_ruleset);
            return -1;
        }
    }

    if(section["rename_node"].size())
        readRegexMatch(section["rename_node"], "@", ext.rename, gAPIMode);

    ext.add_emoji = safe_as<std::string>(section["add_emoji"]);
    ext.remove_old_emoji = safe_as<std::string>(section["remove_old_emoji"]);
    const char *emoji_name = section["emojis"].IsDefined() ? "emojis" : "emoji";
    if(section[emoji_name].size())
        readEmoji(section[emoji_name], ext.emoji, gAPIMode);

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
    std::string base_content, proxy = parseProxy(gProxyConfig), config = fetchFile(path, proxy, gCacheConfig);
    if(render_template(config, *ext.tpl_args, base_content, gTemplatePath) != 0)
        base_content = config;

    try
    {
        YAML::Node yaml = YAML::Load(base_content);
        if(yaml.size() && yaml["custom"].IsDefined())
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
        importItems(ext.custom_proxy_group, gAPIMode);
    }
    std::string ruleset_name = ini.ItemPrefixExist("ruleset") ? "ruleset" : "surge_ruleset";
    if(ini.ItemPrefixExist(ruleset_name))
    {
        ini.GetAll(ruleset_name, ext.surge_ruleset);
        importItems(ext.surge_ruleset, gAPIMode);
        if(gMaxAllowedRulesets && ext.surge_ruleset.size() > gMaxAllowedRulesets)
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
        importItems(ext.rename, gAPIMode);
    }
    ext.add_emoji = ini.Get("add_emoji");
    ext.remove_old_emoji = ini.Get("remove_old_emoji");
    if(ini.ItemPrefixExist("emoji"))
    {
        ini.GetAll("emoji", ext.emoji);
        importItems(ext.emoji, gAPIMode);
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
    if(isLink(path) || (startsWith(path, gBasePath) && fileExist(path)))
        dest = path;
}

std::string subconverter(RESPONSE_CALLBACK_ARGS)
{
    std::string &argument = request.argument;
    int *status_code = &response.status_code;

    std::string argTarget = getUrlArg(argument, "target"), argSurgeVer = getUrlArg(argument, "ver");
    tribool argClashNewField = getUrlArg(argument, "new_name");
    int intSurgeVer = argSurgeVer.size() ? to_int(argSurgeVer, 3) : 3;
    if(argTarget == "auto")
        matchUserAgent(request.headers["User-Agent"], argTarget, argClashNewField, intSurgeVer);

    /// don't try to load groups or rulesets when generating simple subscriptions
    bool lSimpleSubscription = false;
    switch(hash_(argTarget))
    {
    case "ss"_hash: case "ssd"_hash: case "ssr"_hash: case "sssub"_hash: case "v2ray"_hash: case "trojan"_hash: case "mixed"_hash:
        lSimpleSubscription = true;
        break;
    case "clash"_hash: case "clashr"_hash: case "surge"_hash: case "quan"_hash: case "quanx"_hash: case "loon"_hash: case "surfboard"_hash: case "mellow"_hash:
        break;
    default:
        *status_code = 400;
        return "Invalid target!";
    }
    //check if we need to read configuration
    if((!gAPIMode || gCFWChildProcess) && !gGeneratorMode)
        readConf();

    /// string values
    std::string argUrl = UrlDecode(getUrlArg(argument, "url"));
    std::string argGroupName = UrlDecode(getUrlArg(argument, "group")), argUploadPath = getUrlArg(argument, "upload_path");
    std::string argIncludeRemark = UrlDecode(getUrlArg(argument, "include")), argExcludeRemark = UrlDecode(getUrlArg(argument, "exclude"));
    std::string argCustomGroups = urlsafe_base64_decode(getUrlArg(argument, "groups")), argCustomRulesets = urlsafe_base64_decode(getUrlArg(argument, "ruleset")), argExternalConfig = UrlDecode(getUrlArg(argument, "config"));
    std::string argDeviceID = getUrlArg(argument, "dev_id"), argFilename = getUrlArg(argument, "filename"), argUpdateInterval = getUrlArg(argument, "interval"), argUpdateStrict = getUrlArg(argument, "strict");
    std::string argRenames = UrlDecode(getUrlArg(argument, "rename"));

    /// switches with default value
    tribool argUpload = getUrlArg(argument, "upload"), argEmoji = getUrlArg(argument, "emoji"), argAddEmoji = getUrlArg(argument, "add_emoji"), argRemoveEmoji = getUrlArg(argument, "remove_emoji");
    tribool argAppendType = getUrlArg(argument, "append_type"), argTFO = getUrlArg(argument, "tfo"), argUDP = getUrlArg(argument, "udp"), argGenNodeList = getUrlArg(argument, "list");
    tribool argSort = getUrlArg(argument, "sort"), argUseSortScript = getUrlArg(argument, "sort_script");
    tribool argGenClashScript = getUrlArg(argument, "script"), argEnableInsert = getUrlArg(argument, "insert");
    tribool argSkipCertVerify = getUrlArg(argument, "scv"), argFilterDeprecated = getUrlArg(argument, "fdn"), argExpandRulesets = getUrlArg(argument, "expand"), argAppendUserinfo = getUrlArg(argument, "append_info");
    tribool argPrependInsert = getUrlArg(argument, "prepend"), argGenClassicalRuleProvider = getUrlArg(argument, "classic"), argTLS13 = getUrlArg(argument, "tls13");

    std::string base_content, output_content;
    string_array lCustomProxyGroups = gCustomProxyGroups, lCustomRulesets = gCustomRulesets, lIncludeRemarks = gIncludeRemarks, lExcludeRemarks = gExcludeRemarks;
    std::vector<ruleset_content> lRulesetContent;
    extra_settings ext;
    std::string subInfo, dummy;
    int interval = argUpdateInterval.size() ? to_int(argUpdateInterval, gUpdateInterval) : gUpdateInterval;
    bool authorized = !gAPIMode || getUrlArg(argument, "token") == gAccessToken, strict = argUpdateStrict.size() ? argUpdateStrict == "true" : gUpdateStrict;

    if(std::find(gRegexBlacklist.cbegin(), gRegexBlacklist.cend(), argIncludeRemark) != gRegexBlacklist.cend() || std::find(gRegexBlacklist.cbegin(), gRegexBlacklist.cend(), argExcludeRemark) != gRegexBlacklist.cend())
        return "Invalid request!";

    /// for external configuration
    std::string lClashBase = gClashBase, lSurgeBase = gSurgeBase, lMellowBase = gMellowBase, lSurfboardBase = gSurfboardBase;
    std::string lQuanBase = gQuanBase, lQuanXBase = gQuanXBase, lLoonBase = gLoonBase, lSSSubBase = gSSSubBase;

    /// validate urls
    argEnableInsert.define(gEnableInsert);
    if(!argUrl.size() && (!gAPIMode || authorized))
        argUrl = gDefaultUrls;
    if((!argUrl.size() && !(gInsertUrls.size() && argEnableInsert)) || !argTarget.size())
    {
        *status_code = 400;
        return "Invalid request!";
    }

    /// load request arguments as template variables
    string_array req_args = split(argument, "&");
    string_map req_arg_map;
    for(std::string &x : req_args)
    {
        string_size pos = x.find("=");
        if(pos == x.npos)
        {
            req_arg_map[x] = "";
            continue;
        }
        if(x.substr(0, pos) == "token")
            continue;
        req_arg_map[x.substr(0, pos)] = x.substr(pos + 1);
    }

    /// save template variables
    template_args tpl_args;
    tpl_args.global_vars = gTemplateVars;
    tpl_args.request_params = req_arg_map;

    /// check for proxy settings
    std::string proxy = parseProxy(gProxySubscription);

    /// check other flags
    ext.append_proxy_type = argAppendType.get(gAppendType);
    if((argTarget == "clash" || argTarget == "clashr") && argGenClashScript.is_undef())
        argExpandRulesets.define(true);

    ext.clash_proxies_style = gClashProxiesStyle;

    /// read preference from argument, assign global var if not in argument
    ext.tfo.parse(argTFO).parse(gTFO);
    ext.udp.parse(argUDP).parse(gUDP);
    ext.skip_cert_verify.parse(argSkipCertVerify).parse(gSkipCertVerify);
    ext.tls13.parse(argTLS13).parse(gTLS13);

    ext.sort_flag = argSort.get(gEnableSort);
    argUseSortScript.define(gSortScript.size() != 0);
    if(ext.sort_flag && argUseSortScript)
        ext.sort_script = gSortScript;
    ext.filter_deprecated = argFilterDeprecated.get(gFilterDeprecated);
    ext.clash_new_field_name = argClashNewField.get(gClashUseNewField);
    ext.clash_script = argGenClashScript.get();
    ext.clash_classical_ruleset = argGenClassicalRuleProvider.get();
    if(!argExpandRulesets)
        ext.clash_new_field_name = true;
    else
        ext.clash_script = false;

    ext.nodelist = argGenNodeList;
    ext.surge_ssr_path = gSurgeSSRPath;
    ext.quanx_dev_id = argDeviceID.size() ? argDeviceID : gQuanXDevID;
    ext.enable_rule_generator = gEnableRuleGen;
    ext.overwrite_original_rules = gOverwriteOriginalRules;
    if(!argExpandRulesets)
        ext.managed_config_prefix = gManagedConfigPrefix;

    //load external configuration
    if(argExternalConfig.empty())
        argExternalConfig = gDefaultExtConfig;
    if(argExternalConfig.size())
    {
        //std::cerr<<"External configuration file provided. Loading...\n";
        writeLog(0, "External configuration file provided. Loading...", LOG_LEVEL_INFO);
        ExternalConfig extconf;
        extconf.tpl_args = &tpl_args;
        if(loadExternalConfig(argExternalConfig, extconf) == 0)
        {
            if(!ext.nodelist)
            {
                checkExternalBase(extconf.sssub_rule_base, lSSSubBase);
                if(!lSimpleSubscription)
                {
                    checkExternalBase(extconf.clash_rule_base, lClashBase);
                    checkExternalBase(extconf.surge_rule_base, lSurgeBase);
                    checkExternalBase(extconf.surfboard_rule_base, lSurfboardBase);
                    checkExternalBase(extconf.mellow_rule_base, lMellowBase);
                    checkExternalBase(extconf.quan_rule_base, lQuanBase);
                    checkExternalBase(extconf.quanx_rule_base, lQuanXBase);
                    checkExternalBase(extconf.loon_rule_base, lLoonBase);

                    if(extconf.surge_ruleset.size())
                        lCustomRulesets = extconf.surge_ruleset;
                    if(extconf.custom_proxy_group.size())
                        lCustomProxyGroups = extconf.custom_proxy_group;
                    ext.enable_rule_generator = extconf.enable_rule_generator;
                    ext.overwrite_original_rules = extconf.overwrite_original_rules;
                }
            }
            if(extconf.rename.size())
                ext.rename_array = extconf.rename;
            if(extconf.emoji.size())
                ext.emoji_array = extconf.emoji;
            if(extconf.include.size())
                lIncludeRemarks = extconf.include;
            if(extconf.exclude.size())
                lExcludeRemarks = extconf.exclude;
            argAddEmoji.define(extconf.add_emoji);
            argRemoveEmoji.define(extconf.remove_old_emoji);
        }

    }
    else
    {
        if(!lSimpleSubscription)
        {
            //loading custom groups
            if(argCustomGroups.size() && !ext.nodelist)
                lCustomProxyGroups = split(argCustomGroups, "@");

            //loading custom rulesets
            if(argCustomRulesets.size() && !ext.nodelist)
                lCustomRulesets = split(argCustomRulesets, "@");
        }
    }
    if(ext.enable_rule_generator && !ext.nodelist && !lSimpleSubscription)
    {
        if(lCustomRulesets != gCustomRulesets)
            refreshRulesets(lCustomRulesets, lRulesetContent);
        else
        {
            if(gUpdateRulesetOnRequest)
                refreshRulesets(gCustomRulesets, gRulesetContent);
            lRulesetContent = gRulesetContent;
        }
    }

    if(!argEmoji.is_undef())
    {
        argAddEmoji.set(argEmoji);
        argRemoveEmoji.set(true);
    }
    ext.add_emoji = argAddEmoji.get(gAddEmoji);
    ext.remove_emoji = argRemoveEmoji.get(gRemoveEmoji);
    if(ext.add_emoji && ext.emoji_array.empty())
        ext.emoji_array = safe_get_emojis();
    if(argRenames.size())
        ext.rename_array = split(argRenames, "`");
    else if(ext.rename_array.empty())
        ext.rename_array = safe_get_renames();

    //check custom include/exclude settings
    if(argIncludeRemark.size() && regValid(argIncludeRemark))
        lIncludeRemarks = string_array{argIncludeRemark};
    if(argExcludeRemark.size() && regValid(argExcludeRemark))
        lExcludeRemarks = string_array{argExcludeRemark};

    //start parsing urls
    string_array stream_temp = safe_get_streams(), time_temp = safe_get_times();

    //loading urls
    string_array urls;
    std::vector<nodeInfo> nodes, insert_nodes;
    int groupID = 0;
    if(gInsertUrls.size() && argEnableInsert)
    {
        groupID = -1;
        urls = split(gInsertUrls, "|");
        importItems(urls, true);
        for(std::string &x : urls)
        {
            x = regTrim(x);
            writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
            if(addNodes(x, insert_nodes, groupID, proxy, lExcludeRemarks, lIncludeRemarks, stream_temp, time_temp, subInfo, authorized, request.headers) == -1)
            {
                if(gSkipFailedLinks)
                    writeLog(0, "The following link doesn't contain any valid node info: " + x, LOG_LEVEL_WARNING);
                else
                {
                    *status_code = 400;
                    return "The following link doesn't contain any valid node info: " + x;
                }
            }
            groupID--;
        }
    }
    urls = split(argUrl, "|");
    importItems(urls, true);
    groupID = 0;
    for(std::string &x : urls)
    {
        x = regTrim(x);
        //std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
        if(addNodes(x, nodes, groupID, proxy, lExcludeRemarks, lIncludeRemarks, stream_temp, time_temp, subInfo, authorized, request.headers) == -1)
        {
            if(gSkipFailedLinks)
                writeLog(0, "The following link doesn't contain any valid node info: " + x, LOG_LEVEL_WARNING);
            else
            {
                *status_code = 400;
                return "The following link doesn't contain any valid node info: " + x;
            }
        }
        groupID++;
    }
    //exit if found nothing
    if(!nodes.size() && !insert_nodes.size())
    {
        *status_code = 400;
        return "No nodes were found!";
    }
    argPrependInsert.define(gPrependInsert);
    if(argPrependInsert)
    {
        std::move(nodes.begin(), nodes.end(), std::back_inserter(insert_nodes));
        nodes.swap(insert_nodes);
    }
    else
    {
        std::move(insert_nodes.begin(), insert_nodes.end(), std::back_inserter(nodes));
    }
    //run filter script
    if(gFilterScript.size())
    {
        if(startsWith(gFilterScript, "path:"))
            gFilterScript = fileGet(gFilterScript.substr(5), false);
        duk_context *ctx = duktape_init();
        if(ctx)
        {
            defer(duk_destroy_heap(ctx);)
            if(duktape_peval(ctx, gFilterScript) == 0)
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
    if(argGroupName.size())
        for(nodeInfo &x : nodes)
            x.group = argGroupName;

    if(subInfo.size() && argAppendUserinfo.get(gAppendUserinfo))
        response.headers.emplace("Subscription-UserInfo", subInfo);

    //do pre-process now
    preprocessNodes(nodes, ext);

    string_array dummy_group;
    std::vector<ruleset_content> dummy_ruleset;
    std::string managed_url = base64_decode(UrlDecode(getUrlArg(argument, "profile_data")));
    if(managed_url.empty())
        managed_url = gManagedConfigPrefix + "/sub?" + argument;

    //std::cerr<<"Generate target: ";
    proxy = parseProxy(gProxyConfig);
    switch(hash_(argTarget))
    {
    case "clash"_hash: case "clashr"_hash:
        writeLog(0, argTarget == "clashr" ? "Generate target: ClashR" : "Generate target: Clash", LOG_LEVEL_INFO);
        tpl_args.local_vars["clash.new_field_name"] = ext.clash_new_field_name ? "true" : "false";
        response.headers["profile-update-interval"] = std::to_string(interval / 3600);
        if(ext.nodelist)
        {
            YAML::Node yamlnode;
            netchToClash(nodes, yamlnode, dummy_group, argTarget == "clashr", ext);
            output_content = YAML::Dump(yamlnode);
        }
        else
        {
            if(render_template(fetchFile(lClashBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            output_content = netchToClash(nodes, base_content, lRulesetContent, lCustomProxyGroups, argTarget == "clashr", ext);
        }

        if(argUpload)
            uploadGist(argTarget, argUploadPath, output_content, false);
        break;
    case "surge"_hash:

        writeLog(0, "Generate target: Surge " + std::to_string(intSurgeVer), LOG_LEVEL_INFO);

        if(ext.nodelist)
        {
            output_content = netchToSurge(nodes, base_content, dummy_ruleset, dummy_group, intSurgeVer, ext);

            if(argUpload)
                uploadGist("surge" + argSurgeVer + "list", argUploadPath, output_content, true);
        }
        else
        {
            if(render_template(fetchFile(lSurgeBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            output_content = netchToSurge(nodes, base_content, lRulesetContent, lCustomProxyGroups, intSurgeVer, ext);

            if(argUpload)
                uploadGist("surge" + argSurgeVer, argUploadPath, output_content, true);

            if(gWriteManagedConfig && gManagedConfigPrefix.size())
                output_content = "#!MANAGED-CONFIG " + managed_url + (interval ? " interval=" + std::to_string(interval) : "") \
                 + " strict=" + std::string(strict ? "true" : "false") + "\n\n" + output_content;
        }
        break;
    case "surfboard"_hash:
        writeLog(0, "Generate target: Surfboard", LOG_LEVEL_INFO);

        if(render_template(fetchFile(lSurfboardBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        output_content = netchToSurge(nodes, base_content, lRulesetContent, lCustomProxyGroups, -3, ext);
        if(argUpload)
            uploadGist("surfboard", argUploadPath, output_content, true);

        if(gWriteManagedConfig && gManagedConfigPrefix.size())
            output_content = "#!MANAGED-CONFIG " + managed_url + (interval ? " interval=" + std::to_string(interval) : "") \
                 + " strict=" + std::string(strict ? "true" : "false") + "\n\n" + output_content;
        break;
    case "mellow"_hash:
        writeLog(0, "Generate target: Mellow", LOG_LEVEL_INFO);

        if(render_template(fetchFile(lMellowBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        output_content = netchToMellow(nodes, base_content, lRulesetContent, lCustomProxyGroups, ext);

        if(argUpload)
            uploadGist("mellow", argUploadPath, output_content, true);
        break;
    case "sssub"_hash:
        writeLog(0, "Generate target: SS Subscription", LOG_LEVEL_INFO);

        if(render_template(fetchFile(lSSSubBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        output_content = netchToSSSub(base_content, nodes, ext);
        if(argUpload)
            uploadGist("sssub", argUploadPath, output_content, false);
        break;
    case "ss"_hash:
        writeLog(0, "Generate target: SS", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 1, ext);
        if(argUpload)
            uploadGist("ss", argUploadPath, output_content, false);
        break;
    case "ssr"_hash:
        writeLog(0, "Generate target: SSR", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 2, ext);
        if(argUpload)
            uploadGist("ssr", argUploadPath, output_content, false);
        break;
    case "v2ray"_hash:
        writeLog(0, "Generate target: v2rayN", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 4, ext);
        if(argUpload)
            uploadGist("v2ray", argUploadPath, output_content, false);
        break;
    case "trojan"_hash:
        writeLog(0, "Generate target: Trojan", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 8, ext);
        if(argUpload)
            uploadGist("trojan", argUploadPath, output_content, false);
        break;
    case "mixed"_hash:
        writeLog(0, "Generate target: Standard Subscription", LOG_LEVEL_INFO);
        output_content = netchToSingle(nodes, 15, ext);
        if(argUpload)
            uploadGist("sub", argUploadPath, output_content, false);
        break;
    case "quan"_hash:
        writeLog(0, "Generate target: Quantumult", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(lQuanBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
        }

        output_content = netchToQuan(nodes, base_content, lRulesetContent, lCustomProxyGroups, ext);

        if(argUpload)
            uploadGist("quan", argUploadPath, output_content, false);
        break;
    case "quanx"_hash:
        writeLog(0, "Generate target: Quantumult X", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(lQuanXBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
        }

        output_content = netchToQuanX(nodes, base_content, lRulesetContent, lCustomProxyGroups, ext);

        if(argUpload)
            uploadGist("quanx", argUploadPath, output_content, false);
        break;
    case "loon"_hash:
        writeLog(0, "Generate target: Loon", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(lLoonBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
        }

        output_content = netchToLoon(nodes, base_content, lRulesetContent, lCustomProxyGroups, ext);

        if(argUpload)
            uploadGist("loon", argUploadPath, output_content, false);
        break;
    case "ssd"_hash:
        writeLog(0, "Generate target: SSD", LOG_LEVEL_INFO);
        output_content = netchToSSD(nodes, argGroupName, subInfo, ext);
        if(argUpload)
            uploadGist("ssd", argUploadPath, output_content, false);
        break;
    default:
        writeLog(0, "Generate target: Unspecified", LOG_LEVEL_INFO);
        *status_code = 500;
        return "Unrecognized target";
    }
    writeLog(0, "Generate completed.", LOG_LEVEL_INFO);
    if(argFilename.size())
        response.headers.emplace("Content-Disposition", "attachment; filename=\"" + argFilename + "\"");
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
    const std::string proxygroup_name = gClashUseNewField ? "proxy-groups" : "Proxy Group", rule_name = gClashUseNewField ? "rules" : "Rule";

    ini.store_any_line = true;

    if(!url.size())
        url = gDefaultUrls;
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

    std::string proxy = parseProxy(gProxyConfig);
    YAML::Node clash;
    template_args tpl_args;
    tpl_args.global_vars = gTemplateVars;
    tpl_args.local_vars["clash.new_field_name"] = gClashUseNewField ? "true" : "false";
    tpl_args.request_params["target"] = "clash";
    tpl_args.request_params["url"] = url;

    if(render_template(fetchFile(gClashBase, proxy, gCacheConfig), tpl_args, base_content, gTemplatePath) != 0)
    {
        *status_code = 400;
        return base_content;
    }
    clash = YAML::Load(base_content);

    base_content = fetchFile(url, proxy, gCacheConfig);

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
            if(startsWith(dummy_str_array[i], "url"))
                singlegroup["url"] = trim(dummy_str_array[i].substr(dummy_str_array[i].find("=") + 1));
            else if(startsWith(dummy_str_array[i], "interval"))
                singlegroup["interval"] = trim(dummy_str_array[i].substr(dummy_str_array[i].find("=") + 1));
            else if(startsWith(dummy_str_array[i], "policy-path"))
                links.emplace_back(trim(dummy_str_array[i].substr(dummy_str_array[i].find("=") + 1)));
            else
                singlegroup["proxies"].push_back(trim(dummy_str_array[i]));
        }
        clash[proxygroup_name].push_back(singlegroup);
    }

    proxy = parseProxy(gProxySubscription);
    eraseElements(dummy_str_array);

    std::string subInfo;
    for(std::string &x : links)
    {
        //std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
        if(addNodes(x, nodes, 0, proxy, dummy_str_array, dummy_str_array, dummy_str_array, dummy_str_array, subInfo, !gAPIMode, request.headers) == -1)
        {
            if(gSkipFailedLinks)
                writeLog(0, "The following link doesn't contain any valid node info: " + x, LOG_LEVEL_WARNING);
            else
            {
                *status_code = 400;
                return "The following link doesn't contain any valid node info: " + x;
            }
        }
    }

    //exit if found nothing
    if(!nodes.size())
    {
        *status_code = 400;
        return "No nodes were found!";
    }

    extra_settings ext = {true, true, dummy_str_array, dummy_str_array, false, false, false, false, gEnableSort, gFilterDeprecated, gClashUseNewField, false, "", "", "", gUDP, gTFO, gSkipCertVerify, gTLS13};
    ext.clash_proxies_style = gClashProxiesStyle;

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
        if(startsWith(x, "RULE-SET"))
        {
            strArray = split(x, ",");
            if(strArray.size() != 3)
                continue;
            content = webGet(strArray[1], proxy, gCacheRuleset);
            if(!content.size())
                continue;

            ss << content;
            char delimiter = getLineBreak(content);

            while(getline(ss, strLine, delimiter))
            {
                lineSize = strLine.size();
                if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
                    strLine.erase(--lineSize);
                if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                    continue;
                else if(!std::any_of(ClashRuleTypes.begin(), ClashRuleTypes.end(), [&strLine](std::string type){return startsWith(strLine, type);})) //remove unsupported types
                    continue;
                strLine += strArray[2];
                if(count_least(strLine, ',', 3))
                    strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
                rule.push_back(strLine);
            }
            ss.clear();
            continue;
        }
        else if(!std::any_of(ClashRuleTypes.begin(), ClashRuleTypes.end(), [&strLine](std::string type){return startsWith(strLine, type);}))
            continue;
        rule.push_back(x);
    }
    clash[rule_name] = rule;

    response.headers["profile-update-interval"] = std::to_string(gUpdateInterval / 3600);
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
        token = gAccessToken;
    }
    else
    {
        if(token != gAccessToken)
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
    contents.emplace("profile_data", base64_encode(gManagedConfigPrefix + "/getprofile?" + argument));
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

    std::string proxy = parseProxy(gProxyConfig);

    output_content = fetchFile(url, proxy, gCacheConfig);

    if(!dev_id.size())
        dev_id = gQuanXDevID;

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

    std::string proxy = parseProxy(gProxyConfig);

    output_content = fetchFile(url, proxy, gCacheConfig);

    if(!dev_id.size())
        dev_id = gQuanXDevID;

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
                strLine.erase(--lineSize);

            if(!strLine.empty() && regMatch(strLine, pattern))
            {
                url = gManagedConfigPrefix + "/qx-script?id=" + dev_id + "&url=" + urlsafe_base64_encode(regReplace(strLine, pattern, "$2"));
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

    std::string input_content, output_content, proxy = parseProxy(gProxyConfig);
    for(std::string &x : urls)
    {
        input_content = webGet(x, proxy, gCacheConfig);
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
    std::string data = args.at(0)->get<std::string>(), proxy = parseProxy(gProxyConfig);
    writeLog(0, "Template called fetch with url '" + data + "'.", LOG_LEVEL_INFO);
    return webGet(data, proxy, gCacheConfig);
}

std::string jinja2_webGet(const std::string &url)
{
    std::string proxy = parseProxy(gProxyConfig);
    writeLog(0, "Template called fetch with url '" + url + "'.", LOG_LEVEL_INFO);
    return webGet(url, proxy, gCacheConfig);
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
    if(gGenerateProfiles.size())
    {
        //std::cerr<<"Generating with specific artifacts: \""<<gen_profile<<"\"...\n";
        writeLog(0, "Generating with specific artifacts: \"" + gGenerateProfiles + "\"...", LOG_LEVEL_INFO);
        string_array targets = split(gGenerateProfiles, ","), new_targets;
        for(std::string &x : targets)
        {
            x = trim(x);
            if(std::find(sections.cbegin(), sections.cend(), x) != sections.cend())
                new_targets.emplace_back(std::move(x));
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
    std::string proxy = parseProxy(gProxySubscription);
    Request request;
    Response response;
    for(std::string &x : sections)
    {
        arguments.clear();
        response.status_code = 200;
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
            request.argument = "name=" + UrlEncode(profile) + "&token=" + gAccessToken + "&expand=true";
            content = getProfile(request, response);
        }
        else
        {
            if(ini.GetBool("direct") == true)
            {
                std::string url = ini.Get("url");
                content = fetchFile(url, proxy, gCacheSubscription);
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
        if(response.status_code != 200)
        {
            //std::cerr<<"Artifact '"<<x<<"' generate ERROR! Reason: "<<content<<"\n\n";
            writeLog(0, "Artifact '" + x + "' generate ERROR! Reason: " + content + "\n", LOG_LEVEL_ERROR);
            if(sections.size() == 1)
                return -1;
            continue;
        }
        fileWrite(path, content, true);
        auto iter = std::find_if(response.headers.begin(), response.headers.end(), [](auto y){ return y.first == "Subscription-UserInfo"; });
        if(iter != response.headers.end())
            writeLog(0, "User Info for artifact '" + x + "': " + subInfoToMessage(iter->second), LOG_LEVEL_INFO);
        //std::cerr<<"Artifact '"<<x<<"' generate SUCCESS!\n\n";
        writeLog(0, "Artifact '" + x + "' generate SUCCESS!\n", LOG_LEVEL_INFO);
        eraseElements(response.headers);
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

    if(!startsWith(path, gTemplatePath) || !fileExist(path))
    {
        *status_code = 404;
        return "Not found";
    }
    std::string template_content = fetchFile(path, parseProxy(gProxyConfig), gCacheConfig);
    if(template_content.empty())
    {
        *status_code = 400;
        return "File empty or out of scope";
    }
    template_args tpl_args;
    tpl_args.global_vars = gTemplateVars;

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
    if(render_template(template_content, tpl_args, output_content, gTemplatePath) != 0)
    {
        *status_code = 400;
        writeLog(0, "Render failed with error.", LOG_LEVEL_WARNING);
    }
    else
        writeLog(0, "Render completed.", LOG_LEVEL_INFO);

    return output_content;
}
