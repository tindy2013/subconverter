#include <iostream>
#include <string>
#include <mutex>
#include <numeric>

#include <inja.hpp>
#include <yaml-cpp/yaml.h>

#include "../config/binding.h"
#include "../generator/config/nodemanip.h"
#include "../generator/config/ruleconvert.h"
#include "../generator/config/subexport.h"
#include "../generator/template/templates.h"
#include "../script/cron.h"
#include "../script/script_quickjs.h"
#include "../server/webserver.h"
#include "../utils/base64/base64.h"
#include "../utils/file_extra.h"
#include "../utils/ini_reader/ini_reader.h"
#include "../utils/logger.h"
#include "../utils/network.h"
#include "../utils/regexp.h"
#include "../utils/stl_extra.h"
#include "../utils/string.h"
#include "../utils/string_hash.h"
#include "../utils/system.h"
#include "../utils/system.h"
#include "../utils/urlencode.h"
#include "../utils/yamlcpp_extra.h"
#include "interfaces.h"
#include "multithread.h"
#include "settings.h"
#include "upload.h"
#include "webget.h"

extern WebServer webServer;

string_array gRegexBlacklist = {"(.*)*"};

void refreshRulesets(RulesetConfigs &ruleset_list, std::vector<RulesetContent> &ruleset_content_array);

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

std::string getConvertedRuleset(RESPONSE_CALLBACK_ARGS)
{
    std::string url = urlDecode(getUrlArg(request.argument, "url")), type = getUrlArg(request.argument, "type");
    return convertRuleset(fetchFile(url, parseProxy(global.proxyRuleset), global.cacheRuleset), to_int(type));
}

std::string getRuleset(RESPONSE_CALLBACK_ARGS)
{
    std::string &argument = request.argument;
    int *status_code = &response.status_code;
    /// type: 1 for Surge, 2 for Quantumult X, 3 for Clash domain rule-provider, 4 for Clash ipcidr rule-provider, 5 for Surge DOMAIN-SET, 6 for Clash classical ruleset
    std::string url = urlSafeBase64Decode(getUrlArg(argument, "url")), type = getUrlArg(argument, "type"), group = urlSafeBase64Decode(getUrlArg(argument, "group"));
    std::string output_content, dummy;
    int type_int = to_int(type, 0);

    if(!url.size() || !type.size() || (type_int == 2 && !group.size()) || (type_int < 1 || type_int > 6))
    {
        *status_code = 400;
        return "Invalid request!";
    }

    std::string proxy = parseProxy(global.proxyRuleset);
    string_array vArray = split(url, "|");
    for(std::string &x : vArray)
        x.insert(0, "ruleset,");
    std::vector<RulesetContent> rca;
    RulesetConfigs confs = INIBinding::from<RulesetConfig>::from_ini(vArray);
    refreshRulesets(confs, rca);
    for(RulesetContent &x : rca)
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
        if(strFind(strLine, "//"))
        {
            strLine.erase(strLine.find("//"));
            strLine = trimWhitespace(strLine);
        }
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

void checkExternalBase(const std::string &path, std::string &dest)
{
    if(isLink(path) || (startsWith(path, global.basePath) && fileExist(path)))
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
    if((!global.APIMode || global.CFWChildProcess) && !global.generatorMode)
        readConf();

    /// string values
    std::string argUrl = urlDecode(getUrlArg(argument, "url"));
    std::string argGroupName = urlDecode(getUrlArg(argument, "group")), argUploadPath = getUrlArg(argument, "upload_path");
    std::string argIncludeRemark = urlDecode(getUrlArg(argument, "include")), argExcludeRemark = urlDecode(getUrlArg(argument, "exclude"));
    std::string argCustomGroups = urlSafeBase64Decode(getUrlArg(argument, "groups")), argCustomRulesets = urlSafeBase64Decode(getUrlArg(argument, "ruleset")), argExternalConfig = urlDecode(getUrlArg(argument, "config"));
    std::string argDeviceID = getUrlArg(argument, "dev_id"), argFilename = urlDecode(getUrlArg(argument, "filename")), argUpdateInterval = getUrlArg(argument, "interval"), argUpdateStrict = getUrlArg(argument, "strict");
    std::string argRenames = urlDecode(getUrlArg(argument, "rename")), argFilterScript = urlDecode(getUrlArg(argument, "filter_script"));

    /// switches with default value
    tribool argUpload = getUrlArg(argument, "upload"), argEmoji = getUrlArg(argument, "emoji"), argAddEmoji = getUrlArg(argument, "add_emoji"), argRemoveEmoji = getUrlArg(argument, "remove_emoji");
    tribool argAppendType = getUrlArg(argument, "append_type"), argTFO = getUrlArg(argument, "tfo"), argUDP = getUrlArg(argument, "udp"), argGenNodeList = getUrlArg(argument, "list");
    tribool argSort = getUrlArg(argument, "sort"), argUseSortScript = getUrlArg(argument, "sort_script");
    tribool argGenClashScript = getUrlArg(argument, "script"), argEnableInsert = getUrlArg(argument, "insert");
    tribool argSkipCertVerify = getUrlArg(argument, "scv"), argFilterDeprecated = getUrlArg(argument, "fdn"), argExpandRulesets = getUrlArg(argument, "expand"), argAppendUserinfo = getUrlArg(argument, "append_info");
    tribool argPrependInsert = getUrlArg(argument, "prepend"), argGenClassicalRuleProvider = getUrlArg(argument, "classic"), argTLS13 = getUrlArg(argument, "tls13");

    std::string base_content, output_content;
    ProxyGroupConfigs lCustomProxyGroups = global.customProxyGroups;
    RulesetConfigs lCustomRulesets = global.customRulesets;
    string_array lIncludeRemarks = global.includeRemarks, lExcludeRemarks = global.excludeRemarks;
    std::vector<RulesetContent> lRulesetContent;
    extra_settings ext;
    std::string subInfo, dummy;
    int interval = argUpdateInterval.size() ? to_int(argUpdateInterval, global.updateInterval) : global.updateInterval;
    bool authorized = !global.APIMode || getUrlArg(argument, "token") == global.accessToken, strict = argUpdateStrict.size() ? argUpdateStrict == "true" : global.updateStrict;

    if(std::find(gRegexBlacklist.cbegin(), gRegexBlacklist.cend(), argIncludeRemark) != gRegexBlacklist.cend() || std::find(gRegexBlacklist.cbegin(), gRegexBlacklist.cend(), argExcludeRemark) != gRegexBlacklist.cend())
        return "Invalid request!";

    /// for external configuration
    std::string lClashBase = global.clashBase, lSurgeBase = global.surgeBase, lMellowBase = global.mellowBase, lSurfboardBase = global.surfboardBase;
    std::string lQuanBase = global.quanBase, lQuanXBase = global.quanXBase, lLoonBase = global.loonBase, lSSSubBase = global.SSSubBase;

    /// validate urls
    argEnableInsert.define(global.enableInsert);
    if(!argUrl.size() && (!global.APIMode || authorized))
        argUrl = global.defaultUrls;
    if((!argUrl.size() && !(global.insertUrls.size() && argEnableInsert)) || !argTarget.size())
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
    req_arg_map["target"] = argTarget;
    req_arg_map["ver"] = std::to_string(intSurgeVer);

    /// save template variables
    template_args tpl_args;
    tpl_args.global_vars = global.templateVars;
    tpl_args.request_params = req_arg_map;

    /// check for proxy settings
    std::string proxy = parseProxy(global.proxySubscription);

    /// check other flags
    ext.authorized = authorized;
    ext.append_proxy_type = argAppendType.get(global.appendType);
    if((argTarget == "clash" || argTarget == "clashr") && argGenClashScript.is_undef())
        argExpandRulesets.define(true);

    ext.clash_proxies_style = global.clashProxiesStyle;

    /// read preference from argument, assign global var if not in argument
    ext.tfo.define(argTFO).define(global.TFOFlag);
    ext.udp.define(argUDP).define(global.UDPFlag);
    ext.skip_cert_verify.define(argSkipCertVerify).define(global.skipCertVerify);
    ext.tls13.define(argTLS13).define(global.TLS13Flag);

    ext.sort_flag = argSort.get(global.enableSort);
    argUseSortScript.define(global.sortScript.size() != 0);
    if(ext.sort_flag && argUseSortScript)
        ext.sort_script = global.sortScript;
    ext.filter_deprecated = argFilterDeprecated.get(global.filterDeprecated);
    ext.clash_new_field_name = argClashNewField.get(global.clashUseNewField);
    ext.clash_script = argGenClashScript.get();
    ext.clash_classical_ruleset = argGenClassicalRuleProvider.get();
    if(!argExpandRulesets)
        ext.clash_new_field_name = true;
    else
        ext.clash_script = false;

    ext.nodelist = argGenNodeList;
    ext.surge_ssr_path = global.surgeSSRPath;
    ext.quanx_dev_id = argDeviceID.size() ? argDeviceID : global.quanXDevID;
    ext.enable_rule_generator = global.enableRuleGen;
    ext.overwrite_original_rules = global.overwriteOriginalRules;
    if(!argExpandRulesets)
        ext.managed_config_prefix = global.managedConfigPrefix;

    /// load external configuration
    if(argExternalConfig.empty())
        argExternalConfig = global.defaultExtConfig;
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
            /// loading custom groups
            if(argCustomGroups.size() && !ext.nodelist)
            {
                string_array vArray = split(argCustomGroups, "@");
                lCustomProxyGroups = INIBinding::from<ProxyGroupConfig>::from_ini(vArray);
            }

            /// loading custom rulesets
            if(argCustomRulesets.size() && !ext.nodelist)
            {
                string_array vArray = split(argCustomRulesets, "@");
                lCustomRulesets = INIBinding::from<RulesetConfig>::from_ini(vArray);
            }
        }
    }
    if(ext.enable_rule_generator && !ext.nodelist && !lSimpleSubscription)
    {
        if(lCustomRulesets != global.customRulesets)
            refreshRulesets(lCustomRulesets, lRulesetContent);
        else
        {
            if(global.updateRulesetOnRequest)
                refreshRulesets(global.customRulesets, global.rulesetsContent);
            lRulesetContent = global.rulesetsContent;
        }
    }

    if(!argEmoji.is_undef())
    {
        argAddEmoji.set(argEmoji);
        argRemoveEmoji.set(true);
    }
    ext.add_emoji = argAddEmoji.get(global.addEmoji);
    ext.remove_emoji = argRemoveEmoji.get(global.removeEmoji);
    if(ext.add_emoji && ext.emoji_array.empty())
        ext.emoji_array = safe_get_emojis();
    if(argRenames.size())
        ext.rename_array = INIBinding::from<RegexMatchConfig>::from_ini(split(argRenames, "`"), "@");
    else if(ext.rename_array.empty())
        ext.rename_array = safe_get_renames();

    /// check custom include/exclude settings
    if(argIncludeRemark.size() && regValid(argIncludeRemark))
        lIncludeRemarks = string_array{argIncludeRemark};
    if(argExcludeRemark.size() && regValid(argExcludeRemark))
        lExcludeRemarks = string_array{argExcludeRemark};

    /// initialize script runtime
    if(authorized && !global.scriptCleanContext)
    {
        ext.js_runtime = new qjs::Runtime();
        script_runtime_init(*ext.js_runtime);
        ext.js_context = new qjs::Context(*ext.js_runtime);
        script_context_init(*ext.js_context);
    }

    //start parsing urls
    RegexMatchConfigs stream_temp = safe_get_streams(), time_temp = safe_get_times();

    //loading urls
    string_array urls;
    std::vector<Proxy> nodes, insert_nodes;
    int groupID = 0;

    parse_settings parse_set;
    parse_set.proxy = &proxy;
    parse_set.exclude_remarks = &lExcludeRemarks;
    parse_set.include_remarks = &lIncludeRemarks;
    parse_set.stream_rules = &stream_temp;
    parse_set.time_rules = &time_temp;
    parse_set.sub_info = &subInfo;
    parse_set.authorized = authorized;
    parse_set.request_header = &request.headers;
    parse_set.js_runtime = ext.js_runtime;
    parse_set.js_context = ext.js_context;

    if(global.insertUrls.size() && argEnableInsert)
    {
        groupID = -1;
        urls = split(global.insertUrls, "|");
        importItems(urls, true);
        for(std::string &x : urls)
        {
            x = regTrim(x);
            writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
            if(addNodes(x, insert_nodes, groupID, parse_set) == -1)
            {
                if(global.skipFailedLinks)
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
        if(addNodes(x, nodes, groupID, parse_set) == -1)
        {
            if(global.skipFailedLinks)
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
    if(subInfo.size() && argAppendUserinfo.get(global.appendUserinfo))
        response.headers.emplace("Subscription-UserInfo", subInfo);

    if(request.method == "HEAD")
        return "";

    argPrependInsert.define(global.prependInsert);
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
    std::string filterScript = global.filterScript;
    if(authorized && !argFilterScript.empty())
        filterScript = argFilterScript;
    if(filterScript.size())
    {
        if(startsWith(filterScript, "path:"))
            filterScript = fileGet(filterScript.substr(5), false);
        /*
        duk_context *ctx = duktape_init();
        if(ctx)
        {
            defer(duk_destroy_heap(ctx);)
            if(duktape_peval(ctx, filterScript) == 0)
            {
                auto filter = [&](const Proxy &x)
                {
                    duk_get_global_string(ctx, "filter");
                    duktape_push_Proxy(ctx, x);
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
        */
        script_safe_runner(ext.js_runtime, ext.js_context, [&](qjs::Context &ctx)
        {
            try
            {
                ctx.eval(filterScript);
                auto filter = (std::function<bool(const Proxy&)>) ctx.eval("filter");
                nodes.erase(std::remove_if(nodes.begin(), nodes.end(), filter), nodes.end());
            }
            catch(qjs::exception)
            {
                script_print_stack(ctx);
            }
        }, global.scriptCleanContext);
    }

    //check custom group name
    if(argGroupName.size())
        for(Proxy &x : nodes)
            x.Group = argGroupName;

    //do pre-process now
    preprocessNodes(nodes, ext);

    /*
    //insert node info to template
    int index = 0;
    std::string template_node_prefix;
    for(Proxy &x : nodes)
    {
        template_node_prefix = std::to_string(index) + ".";
        tpl_args.node_list[template_node_prefix + "remarks"] = x.remarks;
        tpl_args.node_list[template_node_prefix + "group"] = x.Group;
        tpl_args.node_list[template_node_prefix + "groupid"] = std::to_string(x.GroupId);
        index++;
    }
    */

    ProxyGroupConfigs dummy_group;
    std::vector<RulesetContent> dummy_ruleset;
    std::string managed_url = base64Decode(urlDecode(getUrlArg(argument, "profile_data")));
    if(managed_url.empty())
        managed_url = global.managedConfigPrefix + "/sub?" + argument;

    //std::cerr<<"Generate target: ";
    proxy = parseProxy(global.proxyConfig);
    switch(hash_(argTarget))
    {
    case "clash"_hash: case "clashr"_hash:
        writeLog(0, argTarget == "clashr" ? "Generate target: ClashR" : "Generate target: Clash", LOG_LEVEL_INFO);
        tpl_args.local_vars["clash.new_field_name"] = ext.clash_new_field_name ? "true" : "false";
        response.headers["profile-update-interval"] = std::to_string(interval / 3600);
        if(ext.nodelist)
        {
            YAML::Node yamlnode;
            proxyToClash(nodes, yamlnode, dummy_group, argTarget == "clashr", ext);
            output_content = YAML::Dump(yamlnode);
        }
        else
        {
            if(render_template(fetchFile(lClashBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            output_content = proxyToClash(nodes, base_content, lRulesetContent, lCustomProxyGroups, argTarget == "clashr", ext);
        }

        if(argUpload)
            uploadGist(argTarget, argUploadPath, output_content, false);
        break;
    case "surge"_hash:

        writeLog(0, "Generate target: Surge " + std::to_string(intSurgeVer), LOG_LEVEL_INFO);

        if(ext.nodelist)
        {
            output_content = proxyToSurge(nodes, base_content, dummy_ruleset, dummy_group, intSurgeVer, ext);

            if(argUpload)
                uploadGist("surge" + argSurgeVer + "list", argUploadPath, output_content, true);
        }
        else
        {
            if(render_template(fetchFile(lSurgeBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
            output_content = proxyToSurge(nodes, base_content, lRulesetContent, lCustomProxyGroups, intSurgeVer, ext);

            if(argUpload)
                uploadGist("surge" + argSurgeVer, argUploadPath, output_content, true);

            if(global.writeManagedConfig && global.managedConfigPrefix.size())
                output_content = "#!MANAGED-CONFIG " + managed_url + (interval ? " interval=" + std::to_string(interval) : "") \
                 + " strict=" + std::string(strict ? "true" : "false") + "\n\n" + output_content;
        }
        break;
    case "surfboard"_hash:
        writeLog(0, "Generate target: Surfboard", LOG_LEVEL_INFO);

        if(render_template(fetchFile(lSurfboardBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        output_content = proxyToSurge(nodes, base_content, lRulesetContent, lCustomProxyGroups, -3, ext);
        if(argUpload)
            uploadGist("surfboard", argUploadPath, output_content, true);

        if(global.writeManagedConfig && global.managedConfigPrefix.size())
            output_content = "#!MANAGED-CONFIG " + managed_url + (interval ? " interval=" + std::to_string(interval) : "") \
                 + " strict=" + std::string(strict ? "true" : "false") + "\n\n" + output_content;
        break;
    case "mellow"_hash:
        writeLog(0, "Generate target: Mellow", LOG_LEVEL_INFO);

        if(render_template(fetchFile(lMellowBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        output_content = proxyToMellow(nodes, base_content, lRulesetContent, lCustomProxyGroups, ext);

        if(argUpload)
            uploadGist("mellow", argUploadPath, output_content, true);
        break;
    case "sssub"_hash:
        writeLog(0, "Generate target: SS Subscription", LOG_LEVEL_INFO);

        if(render_template(fetchFile(lSSSubBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
        {
            *status_code = 400;
            return base_content;
        }
        output_content = proxyToSSSub(base_content, nodes, ext);
        if(argUpload)
            uploadGist("sssub", argUploadPath, output_content, false);
        break;
    case "ss"_hash:
        writeLog(0, "Generate target: SS", LOG_LEVEL_INFO);
        output_content = proxyToSingle(nodes, 1, ext);
        if(argUpload)
            uploadGist("ss", argUploadPath, output_content, false);
        break;
    case "ssr"_hash:
        writeLog(0, "Generate target: SSR", LOG_LEVEL_INFO);
        output_content = proxyToSingle(nodes, 2, ext);
        if(argUpload)
            uploadGist("ssr", argUploadPath, output_content, false);
        break;
    case "v2ray"_hash:
        writeLog(0, "Generate target: v2rayN", LOG_LEVEL_INFO);
        output_content = proxyToSingle(nodes, 4, ext);
        if(argUpload)
            uploadGist("v2ray", argUploadPath, output_content, false);
        break;
    case "trojan"_hash:
        writeLog(0, "Generate target: Trojan", LOG_LEVEL_INFO);
        output_content = proxyToSingle(nodes, 8, ext);
        if(argUpload)
            uploadGist("trojan", argUploadPath, output_content, false);
        break;
    case "mixed"_hash:
        writeLog(0, "Generate target: Standard Subscription", LOG_LEVEL_INFO);
        output_content = proxyToSingle(nodes, 15, ext);
        if(argUpload)
            uploadGist("sub", argUploadPath, output_content, false);
        break;
    case "quan"_hash:
        writeLog(0, "Generate target: Quantumult", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(lQuanBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
        }

        output_content = proxyToQuan(nodes, base_content, lRulesetContent, lCustomProxyGroups, ext);

        if(argUpload)
            uploadGist("quan", argUploadPath, output_content, false);
        break;
    case "quanx"_hash:
        writeLog(0, "Generate target: Quantumult X", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(lQuanXBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
        }

        output_content = proxyToQuanX(nodes, base_content, lRulesetContent, lCustomProxyGroups, ext);

        if(argUpload)
            uploadGist("quanx", argUploadPath, output_content, false);
        break;
    case "loon"_hash:
        writeLog(0, "Generate target: Loon", LOG_LEVEL_INFO);
        if(!ext.nodelist)
        {
            if(render_template(fetchFile(lLoonBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
            {
                *status_code = 400;
                return base_content;
            }
        }

        output_content = proxyToLoon(nodes, base_content, lRulesetContent, lCustomProxyGroups, ext);

        if(argUpload)
            uploadGist("loon", argUploadPath, output_content, false);
        break;
    case "ssd"_hash:
        writeLog(0, "Generate target: SSD", LOG_LEVEL_INFO);
        output_content = proxyToSSD(nodes, argGroupName, subInfo, ext);
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
        response.headers.emplace("Content-Disposition", "attachment; filename=\"" + argFilename + "\"; filename*=utf-8''" + urlEncode(argFilename));
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
    request.argument = "target=clashr&url=" + urlEncode(url);
    return subconverter(request, response);
}

std::string surgeConfToClash(RESPONSE_CALLBACK_ARGS)
{
    std::string &argument = request.argument;
    int *status_code = &response.status_code;

    INIReader ini;
    string_array dummy_str_array;
    std::vector<Proxy> nodes;
    std::string base_content, url = argument.size() <= 5 ? "" : argument.substr(5);
    const std::string proxygroup_name = global.clashUseNewField ? "proxy-groups" : "Proxy Group", rule_name = global.clashUseNewField ? "rules" : "Rule";

    ini.store_any_line = true;

    if(!url.size())
        url = global.defaultUrls;
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

    std::string proxy = parseProxy(global.proxyConfig);
    YAML::Node clash;
    template_args tpl_args;
    tpl_args.global_vars = global.templateVars;
    tpl_args.local_vars["clash.new_field_name"] = global.clashUseNewField ? "true" : "false";
    tpl_args.request_params["target"] = "clash";
    tpl_args.request_params["url"] = url;

    if(render_template(fetchFile(global.clashBase, proxy, global.cacheConfig), tpl_args, base_content, global.templatePath) != 0)
    {
        *status_code = 400;
        return base_content;
    }
    clash = YAML::Load(base_content);

    base_content = fetchFile(url, proxy, global.cacheConfig);

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

    proxy = parseProxy(global.proxySubscription);
    eraseElements(dummy_str_array);

    RegexMatchConfigs dummy_regex_array;
    std::string subInfo;
    parse_settings parse_set;
    parse_set.proxy = &proxy;
    parse_set.exclude_remarks = parse_set.include_remarks = &dummy_str_array;
    parse_set.stream_rules = parse_set.time_rules = &dummy_regex_array;
    parse_set.request_header = &request.headers;
    parse_set.sub_info = &subInfo;
    parse_set.authorized = !global.APIMode;
    for(std::string &x : links)
    {
        //std::cerr<<"Fetching node data from url '"<<x<<"'."<<std::endl;
        writeLog(0, "Fetching node data from url '" + x + "'.", LOG_LEVEL_INFO);
        if(addNodes(x, nodes, 0, parse_set) == -1)
        {
            if(global.skipFailedLinks)
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

    extra_settings ext;
    ext.sort_flag = global.enableSort;
    ext.filter_deprecated = global.filterDeprecated;
    ext.clash_new_field_name = global.clashUseNewField;
    ext.udp = global.UDPFlag;
    ext.tfo = global.TFOFlag;
    ext.skip_cert_verify = global.skipCertVerify;
    ext.tls13 = global.TLS13Flag;
    ext.clash_proxies_style = global.clashProxiesStyle;

    ProxyGroupConfigs dummy_groups;
    proxyToClash(nodes, clash, dummy_groups, false, ext);

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
            content = webGet(strArray[1], proxy, global.cacheRuleset);
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

    response.headers["profile-update-interval"] = std::to_string(global.updateInterval / 3600);
    writeLog(0, "Conversion completed.", LOG_LEVEL_INFO);
    return YAML::Dump(clash);
}

std::string getProfile(RESPONSE_CALLBACK_ARGS)
{
    std::string &argument = request.argument;
    int *status_code = &response.status_code;

    std::string name = urlDecode(getUrlArg(argument, "name")), token = urlDecode(getUrlArg(argument, "token"));
    string_array profiles = split(name, "|");
    name = profiles[0];
    if(token.empty() || name.empty())
    {
        *status_code = 403;
        return "Forbidden";
    }
    std::string profile_content;
    /*if(vfs::vfs_exist(name))
    {
        profile_content = vfs::vfs_get(name);
    }
    else */if(fileExist(name))
    {
        profile_content = fileGet(name, true);
    }
    else
    {
        *status_code = 404;
        return "Profile not found";
    }
    //std::cerr<<"Trying to load profile '" + name + "'.\n";
    writeLog(0, "Trying to load profile '" + name + "'.", LOG_LEVEL_INFO);
    INIReader ini;
    if(ini.Parse(profile_content) != INIREADER_EXCEPTION_NONE && !ini.SectionExist("Profile"))
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
        token = global.accessToken;
    }
    else
    {
        if(token != global.accessToken)
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
    contents.emplace("profile_data", base64Encode(global.managedConfigPrefix + "/getprofile?" + argument));
    std::string query = std::accumulate(contents.begin(), contents.end(), std::string(), [](const std::string &x, auto y){ return x + y.first + "=" + urlEncode(y.second) + "&"; });
    query += argument;
    request.argument = query;
    return subconverter(request, response);
}

/*
std::string jinja2_webGet(const std::string &url)
{
    std::string proxy = parseProxy(global.proxyConfig);
    writeLog(0, "Template called fetch with url '" + url + "'.", LOG_LEVEL_INFO);
    return webGet(url, proxy, global.cacheConfig);
}*/

inline std::string intToStream(unsigned long long stream)
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
    using ull = unsigned long long;
    subinfo = replaceAllDistinct(subinfo, "; ", "&");
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
    if(global.generateProfiles.size())
    {
        //std::cerr<<"Generating with specific artifacts: \""<<gen_profile<<"\"...\n";
        writeLog(0, "Generating with specific artifacts: \"" + global.generateProfiles + "\"...", LOG_LEVEL_INFO);
        string_array targets = split(global.generateProfiles, ","), new_targets;
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
    std::string proxy = parseProxy(global.proxySubscription);
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
            request.argument = "name=" + urlEncode(profile) + "&token=" + global.accessToken + "&expand=true";
            content = getProfile(request, response);
        }
        else
        {
            if(ini.GetBool("direct") == true)
            {
                std::string url = ini.Get("url");
                content = fetchFile(url, proxy, global.cacheSubscription);
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
                arguments += y.first + "=" + urlEncode(y.second) + "&";
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

    std::string path = urlDecode(getUrlArg(argument, "path"));
    writeLog(0, "Trying to render template '" + path + "'...", LOG_LEVEL_INFO);

    if(!startsWith(path, global.templatePath) || !fileExist(path))
    {
        *status_code = 404;
        return "Not found";
    }
    std::string template_content = fetchFile(path, parseProxy(global.proxyConfig), global.cacheConfig);
    if(template_content.empty())
    {
        *status_code = 400;
        return "File empty or out of scope";
    }
    template_args tpl_args;
    tpl_args.global_vars = global.templateVars;

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
    if(render_template(template_content, tpl_args, output_content, global.templatePath) != 0)
    {
        *status_code = 400;
        writeLog(0, "Render failed with error.", LOG_LEVEL_WARNING);
    }
    else
        writeLog(0, "Render completed.", LOG_LEVEL_INFO);

    return output_content;
}
