#include <string>
#include <mutex>
#include <toml.hpp>

#include "config/binding.h"
#include "handler/webget.h"
#include "script/cron.h"
#include "server/webserver.h"
#include "utils/logger.h"
#include "utils/network.h"
#include "interfaces.h"
#include "multithread.h"
#include "settings.h"

//multi-thread lock
std::mutex gMutexConfigure;

Settings global;

extern WebServer webServer;

const std::map<std::string, ruleset_type> RulesetTypes = {{"clash-domain:", RULESET_CLASH_DOMAIN}, {"clash-ipcidr:", RULESET_CLASH_IPCIDR}, {"clash-classic:", RULESET_CLASH_CLASSICAL}, \
            {"quanx:", RULESET_QUANX}, {"surge:", RULESET_SURGE}};

int importItems(string_array &target, bool scope_limit)
{
    string_array result;
    std::stringstream ss;
    std::string path, content, strLine;
    unsigned int itemCount = 0;
    for(std::string &x : target)
    {
        if(x.find("!!import:") == std::string::npos)
        {
            result.emplace_back(x);
            continue;
        }
        path = x.substr(x.find(":") + 1);
        writeLog(0, "Trying to import items from " + path);

        std::string proxy = parseProxy(global.proxyConfig);

        if(fileExist(path))
            content = fileGet(path, scope_limit);
        else if(isLink(path))
            content = webGet(path, proxy, global.cacheConfig);
        else
            writeLog(0, "File not found or not a valid URL: " + path, LOG_LEVEL_ERROR);
        if(content.empty())
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

toml::value parseToml(const std::string &content, const std::string &fname)
{
    std::istringstream is(content);
    return toml::parse(is, fname);
}

void importItems(std::vector<toml::value> &root, const std::string &import_key, bool scope_limit = true)
{
    std::string content;
    std::vector<toml::value> newRoot;
    auto iter = root.begin();
    size_t count = 0;

    std::string proxy = parseProxy(global.proxyConfig);
    while(iter != root.end())
    {
        auto& table = iter->as_table();
        if(table.find("import") == table.end())
            newRoot.emplace_back(std::move(*iter));
        else
        {
            const std::string &path = toml::get<std::string>(table.at("import"));
            writeLog(0, "Trying to import items from " + path);
            if(fileExist(path))
                content = fileGet(path, scope_limit);
            else if(isLink(path))
                content = webGet(path, proxy, global.cacheConfig);
            else
                writeLog(0, "File not found or not a valid URL: " + path, LOG_LEVEL_ERROR);
            if(!content.empty())
            {
                auto items = parseToml(content, path);
                auto list = toml::find<std::vector<toml::value>>(items, import_key);
                count += list.size();
                std::move(list.begin(), list.end(), std::back_inserter(newRoot));
            }
        }
        iter++;
    }
    root.swap(newRoot);
    writeLog(0, "Imported " + std::to_string(count) + " item(s).");
}

void readRegexMatch(YAML::Node node, const std::string &delimiter, string_array &dest, bool scope_limit = true)
{
    for(auto && object : node)
    {
        std::string script, url, match, rep, strLine;
        object["script"] >>= script;
        if(!script.empty())
        {
            dest.emplace_back("!!script:" + script);
            continue;
        }
        object["import"] >>= url;
        if(!url.empty())
        {
            dest.emplace_back("!!import:" + url);
            continue;
        }
        object["match"] >>= match;
        object["replace"] >>= rep;
        if(!match.empty() && !rep.empty())
            strLine = match + delimiter + rep;
        else
            continue;
        dest.emplace_back(std::move(strLine));
    }
    importItems(dest, scope_limit);
}

void readEmoji(YAML::Node node, string_array &dest, bool scope_limit = true)
{
    for(auto && object : node)
    {
        std::string script, url, match, rep, strLine;
        object["script"] >>= script;
        if(!script.empty())
        {
            dest.emplace_back("!!script:" + script);
            continue;
        }
        object["import"] >>= url;
        if(!url.empty())
        {
            url = "!!import:" + url;
            dest.emplace_back(url);
            continue;
        }
        object["match"] >>= match;
        object["emoji"] >>= rep;
        if(!match.empty() && !rep.empty())
            strLine = match + "," + rep;
        else
            continue;
        dest.emplace_back(std::move(strLine));
    }
    importItems(dest, scope_limit);
}

void readGroup(YAML::Node node, string_array &dest, bool scope_limit = true)
{
    for(YAML::Node && object : node)
    {
        string_array tempArray;
        std::string name, type;
        object["import"] >>= name;
        if(!name.empty())
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
        for(std::size_t j = 0; j < object["rule"].size(); j++)
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

        std::string strLine = join(tempArray, "`");
        dest.emplace_back(std::move(strLine));
    }
    importItems(dest, scope_limit);
}

void readRuleset(YAML::Node node, string_array &dest, bool scope_limit = true)
{
    for(auto && object : node)
    {
        std::string strLine, name, url, group, interval;
        object["import"] >>= name;
        if(!name.empty())
        {
            dest.emplace_back("!!import:" + name);
            continue;
        }
        object["ruleset"] >>= url;
        object["group"] >>= group;
        object["rule"] >>= name;
        object["interval"] >>= interval;
        if(!url.empty())
        {
            strLine = group + "," + url;
            if(!interval.empty())
                strLine += "," + interval;
        }
        else if(!name.empty())
            strLine = group + ",[]" + name;
        else
            continue;
        dest.emplace_back(std::move(strLine));
    }
    importItems(dest, scope_limit);
}

void refreshRulesets(RulesetConfigs &ruleset_list, std::vector<RulesetContent> &ruleset_content_array)
{
    eraseElements(ruleset_content_array);
    std::string rule_group, rule_url, rule_url_typed, interval;
    RulesetContent rc;

    std::string proxy = parseProxy(global.proxyRuleset);

    for(RulesetConfig &x : ruleset_list)
    {
        rule_group = x.Group;
        rule_url = x.Url;
        std::string::size_type pos = x.Url.find("[]");
        if(pos != std::string::npos)
        {
            writeLog(0, "Adding rule '" + rule_url.substr(pos + 2) + "," + rule_group + "'.", LOG_LEVEL_INFO);
            rc = {rule_group, "", "", RULESET_SURGE, std::async(std::launch::async, [=](){return rule_url.substr(pos);}), 0};
        }
        else
        {
            ruleset_type type = RULESET_SURGE;
            rule_url_typed = rule_url;
            auto iter = std::find_if(RulesetTypes.begin(), RulesetTypes.end(), [rule_url](auto y){ return startsWith(rule_url, y.first); });
            if(iter != RulesetTypes.end())
            {
                rule_url.erase(0, iter->first.size());
                type = iter->second;
            }
            writeLog(0, "Updating ruleset url '" + rule_url + "' with group '" + rule_group + "'.", LOG_LEVEL_INFO);
            rc = {rule_group, rule_url, rule_url_typed, type, fetchFileAsync(rule_url, proxy, global.cacheRuleset, true, global.asyncFetchRuleset), x.Interval};
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

    section["api_mode"] >> global.APIMode;
    section["api_access_token"] >> global.accessToken;
    if(section["default_url"].IsSequence())
    {
        section["default_url"] >> tempArray;
        if(tempArray.size())
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(a) + "|" + std::move(b);
            });
            global.defaultUrls = strLine;
            eraseElements(tempArray);
        }
    }
    global.enableInsert = safe_as<std::string>(section["enable_insert"]);
    if(section["insert_url"].IsSequence())
    {
        section["insert_url"] >> tempArray;
        if(tempArray.size())
        {
            strLine = std::accumulate(std::next(tempArray.begin()), tempArray.end(), tempArray[0], [](std::string a, std::string b)
            {
                return std::move(a) + "|" + std::move(b);
            });
            global.insertUrls = strLine;
            eraseElements(tempArray);
        }
    }
    section["prepend_insert_url"] >> global.prependInsert;
    if(section["exclude_remarks"].IsSequence())
        section["exclude_remarks"] >> global.excludeRemarks;
    if(section["include_remarks"].IsSequence())
        section["include_remarks"] >> global.includeRemarks;
    global.filterScript = safe_as<bool>(section["enable_filter"]) ? safe_as<std::string>(section["filter_script"]) : "";
    section["base_path"] >> global.basePath;
    section["clash_rule_base"] >> global.clashBase;
    section["surge_rule_base"] >> global.surgeBase;
    section["surfboard_rule_base"] >> global.surfboardBase;
    section["mellow_rule_base"] >> global.mellowBase;
    section["quan_rule_base"] >> global.quanBase;
    section["quanx_rule_base"] >> global.quanXBase;
    section["loon_rule_base"] >> global.loonBase;
    section["sssub_rule_base"] >> global.SSSubBase;
    section["singbox_rule_base"] >> global.singBoxBase;

    section["default_external_config"] >> global.defaultExtConfig;
    section["append_proxy_type"] >> global.appendType;
    section["proxy_config"] >> global.proxyConfig;
    section["proxy_ruleset"] >> global.proxyRuleset;
    section["proxy_subscription"] >> global.proxySubscription;
    section["reload_conf_on_request"] >> global.reloadConfOnRequest;

    if(node["userinfo"].IsDefined())
    {
        section = node["userinfo"];
        if(section["stream_rule"].IsSequence())
        {
            readRegexMatch(section["stream_rule"], "|", tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "|");
            safe_set_streams(configs);
            eraseElements(tempArray);
        }
        if(section["time_rule"].IsSequence())
        {
            readRegexMatch(section["time_rule"], "|", tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "|");
            safe_set_times(configs);
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
        global.UDPFlag.set(safe_as<std::string>(section["udp_flag"]));
        global.TFOFlag.set(safe_as<std::string>(section["tcp_fast_open_flag"]));
        global.skipCertVerify.set(safe_as<std::string>(section["skip_cert_verify_flag"]));
        global.TLS13Flag.set(safe_as<std::string>(section["tls13_flag"]));
        section["sort_flag"] >> global.enableSort;
        section["sort_script"] >> global.sortScript;
        section["filter_deprecated_nodes"] >> global.filterDeprecated;
        section["append_sub_userinfo"] >> global.appendUserinfo;
        section["clash_use_new_field_name"] >> global.clashUseNewField;
        section["clash_proxies_style"] >> global.clashProxiesStyle;
        section["clash_proxy_groups_style"] >> global.clashProxyGroupsStyle;
        section["singbox_add_clash_modes"] >> global.singBoxAddClashModes;
    }

    if(section["rename_node"].IsSequence())
    {
        readRegexMatch(section["rename_node"], "@", tempArray, false);
        auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "@");
        safe_set_renames(configs);
        eraseElements(tempArray);
    }

    if(node["managed_config"].IsDefined())
    {
        section = node["managed_config"];
        section["write_managed_config"] >> global.writeManagedConfig;
        section["managed_config_prefix"] >> global.managedConfigPrefix;
        section["config_update_interval"] >> global.updateInterval;
        section["config_update_strict"] >> global.updateStrict;
        section["quanx_device_id"] >> global.quanXDevID;
    }

    if(node["surge_external_proxy"].IsDefined())
    {
        node["surge_external_proxy"]["surge_ssr_path"] >> global.surgeSSRPath;
        node["surge_external_proxy"]["resolve_hostname"] >> global.surgeResolveHostname;
    }

    if(node["emojis"].IsDefined())
    {
        section = node["emojis"];
        section["add_emoji"] >> global.addEmoji;
        section["remove_old_emoji"] >> global.removeEmoji;
        if(section["rules"].IsSequence())
        {
            readEmoji(section["rules"], tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, ",");
            safe_set_emojis(configs);
            eraseElements(tempArray);
        }
    }

    const char *rulesets_title = node["rulesets"].IsDefined() ? "rulesets" : "ruleset";
    if(node[rulesets_title].IsDefined())
    {
        section = node[rulesets_title];
        section["enabled"] >> global.enableRuleGen;
        if(!global.enableRuleGen)
        {
            global.overwriteOriginalRules = false;
            global.updateRulesetOnRequest = false;
        }
        else
        {
            section["overwrite_original_rules"] >> global.overwriteOriginalRules;
            section["update_ruleset_on_request"] >> global.updateRulesetOnRequest;
        }
        const char *ruleset_title = section["rulesets"].IsDefined() ? "rulesets" : "surge_ruleset";
        if(section[ruleset_title].IsSequence())
        {
            string_array vArray;
            readRuleset(section[ruleset_title], vArray, false);
            global.customRulesets = INIBinding::from<RulesetConfig>::from_ini(vArray);
        }
    }

    const char *groups_title = node["proxy_groups"].IsDefined() ? "proxy_groups" : "proxy_group";
    if(node[groups_title].IsDefined() && node[groups_title]["custom_proxy_group"].IsDefined())
    {
        string_array vArray;
        readGroup(node[groups_title]["custom_proxy_group"], vArray, false);
        global.customProxyGroups = INIBinding::from<ProxyGroupConfig>::from_ini(vArray);
    }

    if(node["template"].IsDefined())
    {
        node["template"]["template_path"] >> global.templatePath;
        if(node["template"]["globals"].IsSequence())
        {
            eraseElements(global.templateVars);
            for(size_t i = 0; i < node["template"]["globals"].size(); i++)
            {
                std::string key, value;
                node["template"]["globals"][i]["key"] >> key;
                node["template"]["globals"][i]["value"] >> value;
                global.templateVars[key] = value;
            }
        }
    }

    if(node["aliases"].IsSequence())
    {
        webServer.reset_redirect();
        for(size_t i = 0; i < node["aliases"].size(); i++)
        {
            std::string uri, target;
            node["aliases"][i]["uri"] >> uri;
            node["aliases"][i]["target"] >> target;
            webServer.append_redirect(uri, target);
        }
    }

    if(node["tasks"].IsSequence())
    {
        string_array vArray;
        for(size_t i = 0; i < node["tasks"].size(); i++)
        {
            std::string name, exp, path, timeout;
            node["tasks"][i]["import"] >> name;
            if(name.size())
            {
                vArray.emplace_back("!!import:" + name);
                continue;
            }
            node["tasks"][i]["name"] >> name;
            node["tasks"][i]["cronexp"] >> exp;
            node["tasks"][i]["path"] >> path;
            node["tasks"][i]["timeout"] >> timeout;
            strLine = name + "`" + exp + "`" + path + "`" + timeout;
            vArray.emplace_back(std::move(strLine));
        }
        importItems(vArray, false);
        global.enableCron = !vArray.empty();
        global.cronTasks = INIBinding::from<CronTaskConfig>::from_ini(vArray);
        refresh_schedule();
    }

    if(node["server"].IsDefined())
    {
        node["server"]["listen"] >> global.listenAddress;
        node["server"]["port"] >> global.listenPort;
        node["server"]["serve_file_root"] >>= webServer.serve_file_root;
        webServer.serve_file = !webServer.serve_file_root.empty();
    }

    if(node["advanced"].IsDefined())
    {
        std::string log_level;
        node["advanced"]["log_level"] >> log_level;
        node["advanced"]["print_debug_info"] >> global.printDbgInfo;
        if(global.printDbgInfo)
            global.logLevel = LOG_LEVEL_VERBOSE;
        else
        {
            switch(hash_(log_level))
            {
            case "warn"_hash:
                global.logLevel = LOG_LEVEL_WARNING;
                break;
            case "error"_hash:
                global.logLevel = LOG_LEVEL_ERROR;
                break;
            case "fatal"_hash:
                global.logLevel = LOG_LEVEL_FATAL;
                break;
            case "verbose"_hash:
                global.logLevel = LOG_LEVEL_VERBOSE;
                break;
            case "debug"_hash:
                global.logLevel = LOG_LEVEL_DEBUG;
                break;
            default:
                global.logLevel = LOG_LEVEL_INFO;
            }
        }
        node["advanced"]["max_pending_connections"] >> global.maxPendingConns;
        node["advanced"]["max_concurrent_threads"] >> global.maxConcurThreads;
        node["advanced"]["max_allowed_rulesets"] >> global.maxAllowedRulesets;
        node["advanced"]["max_allowed_rules"] >> global.maxAllowedRules;
        node["advanced"]["max_allowed_download_size"] >> global.maxAllowedDownloadSize;
        if(node["advanced"]["enable_cache"].IsDefined())
        {
            if(safe_as<bool>(node["advanced"]["enable_cache"]))
            {
                node["advanced"]["cache_subscription"] >> global.cacheSubscription;
                node["advanced"]["cache_config"] >> global.cacheConfig;
                node["advanced"]["cache_ruleset"] >> global.cacheRuleset;
                node["advanced"]["serve_cache_on_fetch_fail"] >> global.serveCacheOnFetchFail;
            }
            else
                global.cacheSubscription = global.cacheConfig = global.cacheRuleset = 0; //disable cache
        }
        node["advanced"]["script_clean_context"] >> global.scriptCleanContext;
        node["advanced"]["async_fetch_ruleset"] >> global.asyncFetchRuleset;
        node["advanced"]["skip_failed_links"] >> global.skipFailedLinks;
    }
    writeLog(0, "Load preference settings in YAML format completed.", LOG_LEVEL_INFO);
}

template <class T, class... U>
void find_if_exist(const toml::value &v, const toml::key &k, T& target, U&&... args)
{
    if(v.contains(k)) target = toml::find<T>(v, k);
    if constexpr (sizeof...(args) > 0) find_if_exist(v, std::forward<U>(args)...);
}

void operate_toml_kv_table(const std::vector<toml::table> &arr, const toml::key &key_name, const toml::key &value_name, std::function<void (const toml::value&, const toml::value&)> binary_op)
{
    for(const toml::table &table : arr)
    {
        const auto &key = table.at(key_name), &value = table.at(value_name);
        binary_op(key, value);
    }
}

void readTOMLConf(toml::value &root)
{
    auto section_common = toml::find(root, "common");
    string_array default_url, insert_url;

    find_if_exist(section_common, "default_url", default_url, "insert_url", insert_url);
    global.defaultUrls = join(default_url, "|");
    global.insertUrls = join(insert_url, "|");

    bool filter = false;
    find_if_exist(section_common,
                  "api_mode", global.APIMode,
                  "api_access_token", global.accessToken,
                  "exclude_remarks", global.excludeRemarks,
                  "include_remarks", global.includeRemarks,
                  "enable_insert", global.enableInsert,
                  "prepend_insert_url", global.prependInsert,
                  "enable_filter", filter,
                  "default_external_config", global.defaultExtConfig,
                  "base_path", global.basePath,
                  "clash_rule_base", global.clashBase,
                  "surge_rule_base", global.surgeBase,
                  "surfboard_rule_base", global.surfboardBase,
                  "mellow_rule_base", global.mellowBase,
                  "quan_rule_base", global.quanBase,
                  "quanx_rule_base", global.quanXBase,
                  "loon_rule_base", global.loonBase,
                  "sssub_rule_base", global.SSSubBase,
                  "singbox_rule_base", global.singBoxBase,
                  "proxy_config", global.proxyConfig,
                  "proxy_ruleset", global.proxyRuleset,
                  "proxy_subscription", global.proxySubscription,
                  "append_proxy_type", global.appendType,
                  "reload_conf_on_request", global.reloadConfOnRequest
    );

    if(filter)
        find_if_exist(section_common, "filter_script", global.filterScript);
    else
        global.filterScript.clear();

    safe_set_streams(toml::find_or<RegexMatchConfigs>(root, "userinfo", "stream_rule", RegexMatchConfigs{}));
    safe_set_times(toml::find_or<RegexMatchConfigs>(root, "userinfo", "time_rule", RegexMatchConfigs{}));

    auto section_node_pref = toml::find(root, "node_pref");

    find_if_exist(section_node_pref,
                  "udp_flag", global.UDPFlag,
                  "tcp_fast_open_flag", global.TFOFlag,
                  "skip_cert_verify_flag", global.skipCertVerify,
                  "tls13_flag", global.TLS13Flag,
                  "sort_flag", global.enableSort,
                  "sort_script", global.sortScript,
                  "filter_deprecated_nodes", global.filterDeprecated,
                  "append_sub_userinfo", global.appendUserinfo,
                  "clash_use_new_field_name", global.clashUseNewField,
                  "clash_proxies_style", global.clashProxiesStyle,
                  "clash_proxy_groups_style", global.clashProxyGroupsStyle,
                  "singbox_add_clash_modes", global.singBoxAddClashModes
    );

    auto renameconfs = toml::find_or<std::vector<toml::value>>(section_node_pref, "rename_node", {});
    importItems(renameconfs, "rename_node", false);
    safe_set_renames(toml::get<RegexMatchConfigs>(toml::value(renameconfs)));

    auto section_managed = toml::find(root, "managed_config");

    find_if_exist(section_managed,
                  "write_managed_config", global.writeManagedConfig,
                  "managed_config_prefix", global.managedConfigPrefix,
                  "config_update_interval", global.updateInterval,
                  "config_update_strict", global.updateStrict,
                  "quanx_device_id", global.quanXDevID
    );

    auto section_surge_external = toml::find(root, "surge_external_proxy");
    find_if_exist(section_surge_external,
                  "surge_ssr_path", global.surgeSSRPath,
                  "resolve_hostname", global.surgeResolveHostname
    );

    auto section_emojis = toml::find(root, "emojis");

    find_if_exist(section_emojis,
                  "add_emoji", global.addEmoji,
                  "remove_old_emoji", global.removeEmoji
    );

    auto emojiconfs = toml::find_or<std::vector<toml::value>>(section_emojis, "emoji", {});
    importItems(emojiconfs, "emoji", false);
    safe_set_emojis(toml::get<RegexMatchConfigs>(toml::value(emojiconfs)));

    auto groups = toml::find_or<std::vector<toml::value>>(root, "custom_groups", {});
    importItems(groups, "custom_groups", false);
    global.customProxyGroups = toml::get<ProxyGroupConfigs>(toml::value(groups));

    auto section_ruleset = toml::find(root, "ruleset");

    find_if_exist(section_ruleset,
                  "enabled", global.enableRuleGen,
                  "overwrite_original_rules", global.overwriteOriginalRules,
                  "update_ruleset_on_request", global.updateRulesetOnRequest
    );

    auto rulesets = toml::find_or<std::vector<toml::value>>(root, "rulesets", {});
    importItems(rulesets, "rulesets", false);
    global.customRulesets = toml::get<RulesetConfigs>(toml::value(rulesets));

    auto section_template = toml::find(root, "template");

    global.templatePath = toml::find_or(section_template, "template_path", "template");

    eraseElements(global.templateVars);
    operate_toml_kv_table(toml::find_or<std::vector<toml::table>>(section_template, "globals", {}), "key", "value", [&](const toml::value &key, const toml::value &value)
    {
        global.templateVars[key.as_string()] = value.as_string();
    });

    webServer.reset_redirect();
    operate_toml_kv_table(toml::find_or<std::vector<toml::table>>(root, "aliases", {}), "uri", "target", [&](const toml::value &key, const toml::value &value)
    {
        webServer.append_redirect(key.as_string(), value.as_string());
    });

    auto tasks = toml::find_or<std::vector<toml::value>>(root, "tasks", {});
    importItems(tasks, "tasks", false);
    global.cronTasks = toml::get<CronTaskConfigs>(toml::value(tasks));
    refresh_schedule();

    auto section_server = toml::find(root, "server");

    find_if_exist(section_server,
                  "listen", global.listenAddress,
                  "port", global.listenPort,
                  "serve_file_root", webServer.serve_file_root
    );
    webServer.serve_file = !webServer.serve_file_root.empty();

    auto section_advanced = toml::find(root, "advanced");

    std::string log_level;
    bool enable_cache = true;
    int cache_subscription = global.cacheSubscription, cache_config = global.cacheConfig, cache_ruleset = global.cacheRuleset;

    find_if_exist(section_advanced,
                  "log_level", log_level,
                  "print_debug_info", global.printDbgInfo,
                  "max_pending_connections", global.maxPendingConns,
                  "max_concurrent_threads", global.maxConcurThreads,
                  "max_allowed_rulesets", global.maxAllowedRulesets,
                  "max_allowed_rules", global.maxAllowedRules,
                  "max_allowed_download_size", global.maxAllowedDownloadSize,
                  "enable_cache", enable_cache,
                  "cache_subscription", cache_subscription,
                  "cache_config", cache_config,
                  "cache_ruleset", cache_ruleset,
                  "script_clean_context", global.scriptCleanContext,
                  "async_fetch_ruleset", global.asyncFetchRuleset,
                  "skip_failed_links", global.skipFailedLinks
    );

    if(global.printDbgInfo)
        global.logLevel = LOG_LEVEL_VERBOSE;
    else
    {
        switch(hash_(log_level))
        {
        case "warn"_hash:
            global.logLevel = LOG_LEVEL_WARNING;
            break;
        case "error"_hash:
            global.logLevel = LOG_LEVEL_ERROR;
            break;
        case "fatal"_hash:
            global.logLevel = LOG_LEVEL_FATAL;
            break;
        case "verbose"_hash:
            global.logLevel = LOG_LEVEL_VERBOSE;
            break;
        case "debug"_hash:
            global.logLevel = LOG_LEVEL_DEBUG;
            break;
        default:
            global.logLevel = LOG_LEVEL_INFO;
        }
    }

    if(enable_cache)
    {
        global.cacheSubscription = cache_subscription;
        global.cacheConfig = cache_config;
        global.cacheRuleset = cache_ruleset;
    }
    else
    {
        global.cacheSubscription = global.cacheConfig = global.cacheRuleset = 0;
    }

    writeLog(0, "Load preference settings in TOML format completed.", LOG_LEVEL_INFO);
}

void readConf()
{
    guarded_mutex guard(gMutexConfigure);
    writeLog(0, "Loading preference settings...", LOG_LEVEL_INFO);

    eraseElements(global.excludeRemarks);
    eraseElements(global.includeRemarks);
    eraseElements(global.customProxyGroups);
    eraseElements(global.customRulesets);

    try
    {
        std::string prefdata = fileGet(global.prefPath, false);
        if(prefdata.find("common:") != std::string::npos)
        {
            YAML::Node yaml = YAML::Load(prefdata);
            if(yaml.size() && yaml["common"])
                return readYAMLConf(yaml);
        }
        toml::value conf = parseToml(prefdata, global.prefPath);
        if(!conf.is_uninitialized() && toml::find_or<int>(conf, "version", 0))
            return readTOMLConf(conf);
    }
    catch (YAML::Exception &e)
    {
        //ignore yaml parse error
        writeLog(0, e.what(), LOG_LEVEL_DEBUG);
        writeLog(0, "Unable to load preference settings as YAML.", LOG_LEVEL_DEBUG);
    }
    catch (toml::exception &e)
    {
        //ignore toml parse error
        writeLog(0, e.what(), LOG_LEVEL_DEBUG);
        writeLog(0, "Unable to load preference settings as TOML.", LOG_LEVEL_DEBUG);
    }

    INIReader ini;
    ini.allow_dup_section_titles = true;
    //ini.do_utf8_to_gbk = true;
    int retVal = ini.parse_file(global.prefPath);
    if(retVal != INIREADER_EXCEPTION_NONE)
    {
        writeLog(0, "Unable to load preference settings as INI. Reason: " + ini.get_last_error(), LOG_LEVEL_FATAL);
        return;
    }

    string_array tempArray;

    ini.enter_section("common");
    ini.get_bool_if_exist("api_mode", global.APIMode);
    ini.get_if_exist("api_access_token", global.accessToken);
    ini.get_if_exist("default_url", global.defaultUrls);
    global.enableInsert = ini.get("enable_insert");
    ini.get_if_exist("insert_url", global.insertUrls);
    ini.get_bool_if_exist("prepend_insert_url", global.prependInsert);
    if(ini.item_prefix_exist("exclude_remarks"))
        ini.get_all("exclude_remarks", global.excludeRemarks);
    if(ini.item_prefix_exist("include_remarks"))
        ini.get_all("include_remarks", global.includeRemarks);
    global.filterScript = ini.get_bool("enable_filter") ? ini.get("filter_script") : "";
    ini.get_if_exist("base_path", global.basePath);
    ini.get_if_exist("clash_rule_base", global.clashBase);
    ini.get_if_exist("surge_rule_base", global.surgeBase);
    ini.get_if_exist("surfboard_rule_base", global.surfboardBase);
    ini.get_if_exist("mellow_rule_base", global.mellowBase);
    ini.get_if_exist("quan_rule_base", global.quanBase);
    ini.get_if_exist("quanx_rule_base", global.quanXBase);
    ini.get_if_exist("loon_rule_base", global.loonBase);
    ini.get_if_exist("sssub_rule_base", global.SSSubBase);
    ini.get_if_exist("singbox_rule_base", global.singBoxBase);
    ini.get_if_exist("default_external_config", global.defaultExtConfig);
    ini.get_bool_if_exist("append_proxy_type", global.appendType);
    ini.get_if_exist("proxy_config", global.proxyConfig);
    ini.get_if_exist("proxy_ruleset", global.proxyRuleset);
    ini.get_if_exist("proxy_subscription", global.proxySubscription);
    ini.get_bool_if_exist("reload_conf_on_request", global.reloadConfOnRequest);

    if(ini.section_exist("surge_external_proxy"))
    {
        ini.enter_section("surge_external_proxy");
        ini.get_if_exist("surge_ssr_path", global.surgeSSRPath);
        ini.get_bool_if_exist("resolve_hostname", global.surgeResolveHostname);
    }

    if(ini.section_exist("node_pref"))
    {
        ini.enter_section("node_pref");
        /*
        ini.GetBoolIfExist("udp_flag", udp_flag);
        ini.get_bool_if_exist("tcp_fast_open_flag", tfo_flag);
        ini.get_bool_if_exist("skip_cert_verify_flag", scv_flag);
        */
        global.UDPFlag.set(ini.get("udp_flag"));
        global.TFOFlag.set(ini.get("tcp_fast_open_flag"));
        global.skipCertVerify.set(ini.get("skip_cert_verify_flag"));
        global.TLS13Flag.set(ini.get("tls13_flag"));
        ini.get_bool_if_exist("sort_flag", global.enableSort);
        global.sortScript = ini.get("sort_script");
        ini.get_bool_if_exist("filter_deprecated_nodes", global.filterDeprecated);
        ini.get_bool_if_exist("append_sub_userinfo", global.appendUserinfo);
        ini.get_bool_if_exist("clash_use_new_field_name", global.clashUseNewField);
        ini.get_if_exist("clash_proxies_style", global.clashProxiesStyle);
        ini.get_if_exist("clash_proxy_groups_style", global.clashProxyGroupsStyle);
        ini.get_bool_if_exist("singbox_add_clash_modes", global.singBoxAddClashModes);
        if(ini.item_prefix_exist("rename_node"))
        {
            ini.get_all("rename_node", tempArray);
            importItems(tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "@");
            safe_set_renames(configs);
            eraseElements(tempArray);
        }
    }

    if(ini.section_exist("userinfo"))
    {
        ini.enter_section("userinfo");
        if(ini.item_prefix_exist("stream_rule"))
        {
            ini.get_all("stream_rule", tempArray);
            importItems(tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "|");
            safe_set_streams(configs);
            eraseElements(tempArray);
        }
        if(ini.item_prefix_exist("time_rule"))
        {
            ini.get_all("time_rule", tempArray);
            importItems(tempArray, false);
            auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, "|");
            safe_set_times(configs);
            eraseElements(tempArray);
        }
    }

    ini.enter_section("managed_config");
    ini.get_bool_if_exist("write_managed_config", global.writeManagedConfig);
    ini.get_if_exist("managed_config_prefix", global.managedConfigPrefix);
    ini.get_int_if_exist("config_update_interval", global.updateInterval);
    ini.get_bool_if_exist("config_update_strict", global.updateStrict);
    ini.get_if_exist("quanx_device_id", global.quanXDevID);

    ini.enter_section("emojis");
    ini.get_bool_if_exist("add_emoji", global.addEmoji);
    ini.get_bool_if_exist("remove_old_emoji", global.removeEmoji);
    if(ini.item_prefix_exist("rule"))
    {
        ini.get_all("rule", tempArray);
        importItems(tempArray, false);
        auto configs = INIBinding::from<RegexMatchConfig>::from_ini(tempArray, ",");
        safe_set_emojis(configs);
        eraseElements(tempArray);
    }

    if(ini.section_exist("rulesets"))
        ini.enter_section("rulesets");
    else
        ini.enter_section("ruleset");
    global.enableRuleGen = ini.get_bool("enabled");
    if(global.enableRuleGen)
    {
        ini.get_bool_if_exist("overwrite_original_rules", global.overwriteOriginalRules);
        ini.get_bool_if_exist("update_ruleset_on_request", global.updateRulesetOnRequest);
        if(ini.item_prefix_exist("ruleset"))
        {
            string_array vArray;
            ini.get_all("ruleset", vArray);
            importItems(vArray, false);
            global.customRulesets = INIBinding::from<RulesetConfig>::from_ini(vArray);
        }
        else if(ini.item_prefix_exist("surge_ruleset"))
        {
            string_array vArray;
            ini.get_all("surge_ruleset", vArray);
            importItems(vArray, false);
            global.customRulesets = INIBinding::from<RulesetConfig>::from_ini(vArray);
        }
    }
    else
    {
        global.overwriteOriginalRules = false;
        global.updateRulesetOnRequest = false;
    }

    if(ini.section_exist("proxy_groups"))
        ini.enter_section("proxy_groups");
    else
        ini.enter_section("clash_proxy_group");
    if(ini.item_prefix_exist("custom_proxy_group"))
    {
        string_array vArray;
        ini.get_all("custom_proxy_group", vArray);
        importItems(vArray, false);
        global.customProxyGroups = INIBinding::from<ProxyGroupConfig>::from_ini(vArray);
    }

    ini.enter_section("template");
    ini.get_if_exist("template_path", global.templatePath);
    string_multimap tempmap;
    ini.get_items(tempmap);
    eraseElements(global.templateVars);
    for(auto &x : tempmap)
    {
        if(x.first == "template_path")
            continue;
        global.templateVars[x.first] = x.second;
    }
    global.templateVars["managed_config_prefix"] = global.managedConfigPrefix;

    if(ini.section_exist("aliases"))
    {
        ini.enter_section("aliases");
        ini.get_items(tempmap);
        webServer.reset_redirect();
        for(auto &x : tempmap)
            webServer.append_redirect(x.first, x.second);
    }

    if(ini.section_exist("tasks"))
    {
        string_array vArray;
        ini.enter_section("tasks");
        ini.get_all("task", vArray);
        importItems(vArray, false);
        global.enableCron = !vArray.empty();
        global.cronTasks = INIBinding::from<CronTaskConfig>::from_ini(vArray);
        refresh_schedule();
    }

    ini.enter_section("server");
    ini.get_if_exist("listen", global.listenAddress);
    ini.get_int_if_exist("port", global.listenPort);
    webServer.serve_file_root = ini.get("serve_file_root");
    webServer.serve_file = !webServer.serve_file_root.empty();

    ini.enter_section("advanced");
    std::string log_level;
    ini.get_if_exist("log_level", log_level);
    ini.get_bool_if_exist("print_debug_info", global.printDbgInfo);
    if(global.printDbgInfo)
        global.logLevel = LOG_LEVEL_VERBOSE;
    else
    {
        switch(hash_(log_level))
        {
        case "warn"_hash:
            global.logLevel = LOG_LEVEL_WARNING;
            break;
        case "error"_hash:
            global.logLevel = LOG_LEVEL_ERROR;
            break;
        case "fatal"_hash:
            global.logLevel = LOG_LEVEL_FATAL;
            break;
        case "verbose"_hash:
            global.logLevel = LOG_LEVEL_VERBOSE;
            break;
        case "debug"_hash:
            global.logLevel = LOG_LEVEL_DEBUG;
            break;
        default:
            global.logLevel = LOG_LEVEL_INFO;
        }
    }
    ini.get_int_if_exist("max_pending_connections", global.maxPendingConns);
    ini.get_int_if_exist("max_concurrent_threads", global.maxConcurThreads);
    ini.get_number_if_exist("max_allowed_rulesets", global.maxAllowedRulesets);
    ini.get_number_if_exist("max_allowed_rules", global.maxAllowedRules);
    ini.get_number_if_exist("max_allowed_download_size", global.maxAllowedDownloadSize);
    if(ini.item_exist("enable_cache"))
    {
        if(ini.get_bool("enable_cache"))
        {
            ini.get_int_if_exist("cache_subscription", global.cacheSubscription);
            ini.get_int_if_exist("cache_config", global.cacheConfig);
            ini.get_int_if_exist("cache_ruleset", global.cacheRuleset);
            ini.get_bool_if_exist("serve_cache_on_fetch_fail", global.serveCacheOnFetchFail);
        }
        else
        {
            global.cacheSubscription = global.cacheConfig = global.cacheRuleset = 0; //disable cache
            global.serveCacheOnFetchFail = false;
        }
    }
    ini.get_bool_if_exist("script_clean_context", global.scriptCleanContext);
    ini.get_bool_if_exist("async_fetch_ruleset", global.asyncFetchRuleset);
    ini.get_bool_if_exist("skip_failed_links", global.skipFailedLinks);

    writeLog(0, "Load preference settings in INI format completed.", LOG_LEVEL_INFO);
}

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
    section["singbox_rule_base"] >> ext.singbox_rule_base;

    section["enable_rule_generator"] >> ext.enable_rule_generator;
    section["overwrite_original_rules"] >> ext.overwrite_original_rules;

    const char *group_name = section["proxy_groups"].IsDefined() ? "proxy_groups" : "custom_proxy_group";
    if(section[group_name].size())
    {
        string_array vArray;
        readGroup(section[group_name], vArray, global.APIMode);
        ext.custom_proxy_group = INIBinding::from<ProxyGroupConfig>::from_ini(vArray);
    }

    const char *ruleset_name = section["rulesets"].IsDefined() ? "rulesets" : "surge_ruleset";
    if(section[ruleset_name].size())
    {
        string_array vArray;
        readRuleset(section[ruleset_name], vArray, global.APIMode);
        if(global.maxAllowedRulesets && vArray.size() > global.maxAllowedRulesets)
        {
            writeLog(0, "Ruleset count in external config has exceeded limit.", LOG_LEVEL_WARNING);
            return -1;
        }
        ext.surge_ruleset = INIBinding::from<RulesetConfig>::from_ini(vArray);
    }

    if(section["rename_node"].size())
    {
        string_array vArray;
        readRegexMatch(section["rename_node"], "@", vArray, global.APIMode);
        ext.rename = INIBinding::from<RegexMatchConfig>::from_ini(vArray, "@");
    }

    ext.add_emoji = safe_as<std::string>(section["add_emoji"]);
    ext.remove_old_emoji = safe_as<std::string>(section["remove_old_emoji"]);
    const char *emoji_name = section["emojis"].IsDefined() ? "emojis" : "emoji";
    if(section[emoji_name].size())
    {
        string_array vArray;
        readEmoji(section[emoji_name], vArray, global.APIMode);
        ext.emoji = INIBinding::from<RegexMatchConfig>::from_ini(vArray, ",");
    }

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

int loadExternalTOML(toml::value &root, ExternalConfig &ext)
{
    auto section = toml::find(root, "custom");

    find_if_exist(section,
                  "enable_rule_generator", ext.enable_rule_generator,
                  "overwrite_original_rules", ext.overwrite_original_rules,
                  "clash_rule_base", ext.clash_rule_base,
                  "surge_rule_base", ext.surge_rule_base,
                  "surfboard_rule_base", ext.surfboard_rule_base,
                  "mellow_rule_base", ext.mellow_rule_base,
                  "quan_rule_base", ext.quan_rule_base,
                  "quanx_rule_base", ext.quanx_rule_base,
                  "loon_rule_base", ext.loon_rule_base,
                  "sssub_rule_base", ext.sssub_rule_base,
                  "singbox_rule_base", ext.singbox_rule_base,
                  "add_emoji", ext.add_emoji,
                  "remove_old_emoji", ext.remove_old_emoji,
                  "include_remarks", ext.include,
                  "exclude_remarks", ext.exclude
    );

    if(ext.tpl_args != nullptr) operate_toml_kv_table(toml::find_or<std::vector<toml::table>>(root, "template_args", {}), "key", "value",
                                                      [&](const toml::value &key, const toml::value &value)
    {
        std::string val = toml::format(value);
        ext.tpl_args->local_vars[key.as_string()] = val;
    });

    auto groups = toml::find_or<std::vector<toml::value>>(root, "custom_groups", {});
    importItems(groups, "custom_groups", false);
    ext.custom_proxy_group = toml::get<ProxyGroupConfigs>(toml::value(groups));

    auto rulesets = toml::find_or<std::vector<toml::value>>(root, "rulesets", {});
    importItems(rulesets, "rulesets", false);
    if(global.maxAllowedRulesets && rulesets.size() > global.maxAllowedRulesets)
    {
        writeLog(0, "Ruleset count in external config has exceeded limit. ", LOG_LEVEL_WARNING);
        return -1;
    }
    ext.surge_ruleset = toml::get<RulesetConfigs>(toml::value(rulesets));

    auto emojiconfs = toml::find_or<std::vector<toml::value>>(root, "emoji", {});
    importItems(emojiconfs, "emoji", false);
    ext.emoji = toml::get<RegexMatchConfigs>(toml::value(emojiconfs));

    auto renameconfs = toml::find_or<std::vector<toml::value>>(root, "rename_node", {});
    importItems(renameconfs, "rename_node", false);
    ext.rename = toml::get<RegexMatchConfigs>(toml::value(renameconfs));

    return 0;
}

int loadExternalConfig(std::string &path, ExternalConfig &ext)
{
    std::string base_content, proxy = parseProxy(global.proxyConfig), config = fetchFile(path, proxy, global.cacheConfig);
    if(render_template(config, *ext.tpl_args, base_content, global.templatePath) != 0)
        base_content = config;

    try
    {
        YAML::Node yaml = YAML::Load(base_content);
        if(yaml.size() && yaml["custom"].IsDefined())
            return loadExternalYAML(yaml, ext);
        toml::value conf = parseToml(base_content, path);
        if(!conf.is_uninitialized() && toml::find_or<int>(conf, "version", 0))
            return loadExternalTOML(conf, ext);
    }
    catch (YAML::Exception &e)
    {
        //ignore
    }
    catch (toml::exception &e)
    {
        //ignore
    }

    INIReader ini;
    ini.store_isolated_line = true;
    ini.set_isolated_items_section("custom");
    if(ini.parse(base_content) != INIREADER_EXCEPTION_NONE)
    {
        //std::cerr<<"Load external configuration failed. Reason: "<<ini.get_last_error()<<"\n";
        writeLog(0, "Load external configuration failed. Reason: " + ini.get_last_error(), LOG_LEVEL_ERROR);
        return -1;
    }

    ini.enter_section("custom");
    if(ini.item_prefix_exist("custom_proxy_group"))
    {
        string_array vArray;
        ini.get_all("custom_proxy_group", vArray);
        importItems(vArray, global.APIMode);
        ext.custom_proxy_group = INIBinding::from<ProxyGroupConfig>::from_ini(vArray);
    }
    std::string ruleset_name = ini.item_prefix_exist("ruleset") ? "ruleset" : "surge_ruleset";
    if(ini.item_prefix_exist(ruleset_name))
    {
        string_array vArray;
        ini.get_all(ruleset_name, vArray);
        importItems(vArray, global.APIMode);
        if(global.maxAllowedRulesets && vArray.size() > global.maxAllowedRulesets)
        {
            writeLog(0, "Ruleset count in external config has exceeded limit. ", LOG_LEVEL_WARNING);
            return -1;
        }
        ext.surge_ruleset = INIBinding::from<RulesetConfig>::from_ini(vArray);
    }

    ini.get_if_exist("clash_rule_base", ext.clash_rule_base);
    ini.get_if_exist("surge_rule_base", ext.surge_rule_base);
    ini.get_if_exist("surfboard_rule_base", ext.surfboard_rule_base);
    ini.get_if_exist("mellow_rule_base", ext.mellow_rule_base);
    ini.get_if_exist("quan_rule_base", ext.quan_rule_base);
    ini.get_if_exist("quanx_rule_base", ext.quanx_rule_base);
    ini.get_if_exist("loon_rule_base", ext.loon_rule_base);
    ini.get_if_exist("sssub_rule_base", ext.sssub_rule_base);
    ini.get_if_exist("singbox_rule_base", ext.singbox_rule_base);

    ini.get_bool_if_exist("overwrite_original_rules", ext.overwrite_original_rules);
    ini.get_bool_if_exist("enable_rule_generator", ext.enable_rule_generator);

    if(ini.item_prefix_exist("rename"))
    {
        string_array vArray;
        ini.get_all("rename", vArray);
        importItems(vArray, global.APIMode);
        ext.rename = INIBinding::from<RegexMatchConfig>::from_ini(vArray, "@");
    }
    ext.add_emoji = ini.get("add_emoji");
    ext.remove_old_emoji = ini.get("remove_old_emoji");
    if(ini.item_prefix_exist("emoji"))
    {
        string_array vArray;
        ini.get_all("emoji", vArray);
        importItems(vArray, global.APIMode);
        ext.emoji = INIBinding::from<RegexMatchConfig>::from_ini(vArray, ",");
    }
    if(ini.item_prefix_exist("include_remarks"))
        ini.get_all("include_remarks", ext.include);
    if(ini.item_prefix_exist("exclude_remarks"))
        ini.get_all("exclude_remarks", ext.exclude);

    if(ini.section_exist("template") && ext.tpl_args != nullptr)
    {
        ini.enter_section("template");
        string_multimap tempmap;
        ini.get_items(tempmap);
        for(auto &x : tempmap)
            ext.tpl_args->local_vars[x.first] = x.second;
    }

    return 0;
}
