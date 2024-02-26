#include <string>
#include <map>
#include <sstream>
#include <filesystem>
#include <inja.hpp>
#include <nlohmann/json.hpp>

#include "handler/interfaces.h"
#include "handler/settings.h"
#include "handler/webget.h"
#include "utils/logger.h"
#include "utils/network.h"
#include "utils/regexp.h"
#include "utils/urlencode.h"
#include "utils/yamlcpp_extra.h"
#include "templates.h"

namespace inja
{
    void convert_dot_to_json_pointer(std::string_view dot, std::string& out)
    {
        out = DataNode::convert_dot_to_ptr(dot);
    }
}

static inline void parse_json_pointer(nlohmann::json &json, const std::string &path, const std::string &value)
{
    std::string pointer;
    inja::convert_dot_to_json_pointer(path, pointer);
    try
    {
        json[nlohmann::json::json_pointer(pointer)] = value;
    }
    catch (std::exception&)
    {
        //ignore broken pointer
    }
}

/*
std::string parseHostname(inja::Arguments &args)
{
    std::string data = args.at(0)->get<std::string>(), hostname;
    const std::string matcher = R"(^(?i:hostname\s*?=\s*?)(.*?)\s$)";
    string_array urls = split(data, ",");
    if(!urls.size())
        return std::string();

    std::string input_content, output_content, proxy = parseProxy(global.proxyConfig);
    for(std::string &x : urls)
    {
        input_content = webGet(x, proxy, global.cacheConfig);
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
}*/

#ifndef NO_WEBGET
std::string template_webGet(inja::Arguments &args)
{
    std::string data = args.at(0)->get<std::string>(), proxy = parseProxy(global.proxyConfig);
    writeLog(0, "Template called fetch with url '" + data + "'.", LOG_LEVEL_INFO);
    return webGet(data, proxy, global.cacheConfig);
}
#endif // NO_WEBGET

int render_template(const std::string &content, const template_args &vars, std::string &output, const std::string &include_scope)
{
    std::string absolute_scope;
    try
    {
        if(!include_scope.empty())
            absolute_scope = std::filesystem::canonical(include_scope).string();
    }
    catch(std::exception &e)
    {
        writeLog(0, e.what(), LOG_LEVEL_ERROR);
    }
    nlohmann::json data;
    for(auto &x : vars.global_vars)
        parse_json_pointer(data["global"], x.first, x.second);
    std::string all_args;
    for(auto &x : vars.request_params)
    {
        all_args += x.first;
        if(!x.second.empty())
        {
            parse_json_pointer(data["request"], x.first, x.second);
            all_args += "=" + x.second;
        }
        all_args += "&";
    }
    all_args.erase(all_args.size() - 1);
    parse_json_pointer(data["request"], "_args", all_args);
    for(auto &x : vars.local_vars)
        parse_json_pointer(data["local"], x.first, x.second);

    inja::Environment env;

    env.set_trim_blocks(true);
    env.set_lstrip_blocks(true);
    env.set_line_statement("#~#");
    env.add_callback("UrlEncode", 1, [](inja::Arguments &args)
    {
        std::string data = args.at(0)->get<std::string>();
        return urlEncode(data);
    });
    env.add_callback("UrlDecode", 1, [](inja::Arguments &args)
    {
        std::string data = args.at(0)->get<std::string>();
        return urlDecode(data);
    });
    env.add_callback("trim_of", 2, [](inja::Arguments &args)
    {
        std::string data = args.at(0)->get<std::string>(), target = args.at(1)->get<std::string>();
        if(target.empty())
            return data;
        return trimOf(data, target[0]);
    });
    env.add_callback("trim", 1, [](inja::Arguments &args)
    {
        std::string data = args.at(0)->get<std::string>();
        return trim(data);
    });
    env.add_callback("find", 2, [](inja::Arguments &args)
    {
        std::string src = args.at(0)->get<std::string>(), target = args.at(1)->get<std::string>();
        return regFind(src, target);
    });
    env.add_callback("replace", 3, [](inja::Arguments &args)
    {
        std::string src = args.at(0)->get<std::string>(), target = args.at(1)->get<std::string>(), rep = args.at(2)->get<std::string>();
        if(target.empty() || src.empty())
            return src;
        return regReplace(src, target, rep);
    });
    env.add_callback("set", 2, [&data](inja::Arguments &args)
    {
        std::string key = args.at(0)->get<std::string>(), value = args.at(1)->get<std::string>();
        parse_json_pointer(data, key, value);
        return "";
    });
    env.add_callback("split", 3, [&data](inja::Arguments &args)
    {
        std::string content = args.at(0)->get<std::string>(), delim = args.at(1)->get<std::string>(), dest = args.at(2)->get<std::string>();
        string_array vArray = split(content, delim);
        for(size_t index = 0; index < vArray.size(); index++)
            parse_json_pointer(data, dest + "." + std::to_string(index), vArray[index]);
        return "";
    });
    env.add_callback("append", 2, [&data](inja::Arguments &args)
    {
        std::string path = args.at(0)->get<std::string>(), value = args.at(1)->get<std::string>(), pointer, output_content;
        inja::convert_dot_to_json_pointer(path, pointer);
        try
        {
            output_content = data[nlohmann::json::json_pointer(pointer)].get<std::string>();
        }
        catch (std::exception &e)
        {
            // non-exist path, ignore
        }
        output_content.append(value);
        data[nlohmann::json::json_pointer(pointer)] = output_content;
        return "";
    });
    env.add_callback("getLink", 1, [](inja::Arguments &args)
    {
        return global.managedConfigPrefix + args.at(0)->get<std::string>();
    });
    env.add_callback("startsWith", 2, [](inja::Arguments &args)
    {
        return startsWith(args.at(0)->get<std::string>(), args.at(1)->get<std::string>());
    });
    env.add_callback("endsWith", 2, [](inja::Arguments &args)
    {
        return endsWith(args.at(0)->get<std::string>(), args.at(1)->get<std::string>());
    });
    env.add_callback("or", -1, [](inja::Arguments &args)
    {
        for(auto iter = args.begin(); iter != args.end(); iter++)
            if((*iter)->get<int>())
                return true;
        return false;
    });
    env.add_callback("and", -1, [](inja::Arguments &args)
    {
        for(auto iter = args.begin(); iter != args.end(); iter++)
            if(!(*iter)->get<int>())
                return false;
        return true;
    });
    env.add_callback("bool", 1, [](inja::Arguments &args)
    {
        std::string value = args.at(0)->get<std::string>();
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
        switch(hash_(value))
        {
        case "true"_hash:
        case "1"_hash:
            return 1;
        default:
            return 0;
        }
    });
    env.add_callback("string", 1, [](inja::Arguments &args)
    {
        return std::to_string(args.at(0)->get<int>());
    });
#ifndef NO_WEBGET
    env.add_callback("fetch", 1, template_webGet);
#endif // NO_WEBGET
    //env.add_callback("parseHostname", 1, parseHostname);

    env.set_include_callback([&](const std::string &name, const std::string &template_name)
    {
        std::string absolute_path;
        try
        {
            absolute_path = std::filesystem::canonical(template_name).string();
        }
        catch(std::exception &e)
        {
            throw inja::FileError(e.what());
        }
        if(!absolute_scope.empty() && !startsWith(absolute_path, absolute_scope))
            throw inja::FileError("access denied when trying to include '" + template_name + "': out of scope");
        return env.parse(fileGet(template_name, true));
    });
    env.set_search_included_templates_in_files(false);

    try
    {
        std::stringstream out;
        env.render_to(out, env.parse(content), data);
        output = out.str();
        return 0;
    }
    catch (std::exception &e)
    {
        output = "Template render failed! Reason: " + std::string(e.what());
        writeLog(0, output, LOG_LEVEL_ERROR);
        return -1;
    }
    return -2;
}

const std::string clash_script_template = R"(def main(ctx, md):
  host = md["host"]
{% for rule in rules %}
{% if rule.set == "true" %}{% include "group_template" %}{% endif %}
{% endfor %}

{% if exists("geoips") %}  geoips = { {{ geoips }} }
  ip = md["dst_ip"]
  if ip == "":
    ip = ctx.resolve_ip(host)
    if ip == "":
      ctx.log('[Script] dns lookup error use {{ match_group }}')
      return "{{ match_group }}"
  for key in geoips:
    if ctx.geoip(ip) == key:
      return geoips[key]{% endif %}
  return "{{ match_group }}")";

const std::string clash_script_group_template = R"({% if (rule.has_domain == "false" and rule.has_ipcidr == "false") or rule.original == "true" %}  if ctx.rule_providers["{{ rule.name }}"].match(md):
    ctx.log('[Script] matched {{ rule.group }} rule')
    return "{{ rule.group }}"{% else %}{% if rule.has_domain == "true" %}  if ctx.rule_providers["{{ rule.name }}_domain"].match(md):
    ctx.log('[Script] matched {{ rule.group }} DOMAIN rule')
    return "{{ rule.group }}"{% endif %}
{% if not rule.keyword == "" %}{% include "keyword_template" %}{% endif %}
{% if rule.has_ipcidr == "true" %}  if ctx.rule_providers["{{ rule.name }}_ipcidr"].match(md):
    ctx.log('[Script] matched {{ rule.group }} IP rule')
    return "{{ rule.group }}"{% endif %}{% endif %})";

const std::string clash_script_keyword_template = R"(  keywords = [{{ rule.keyword }}]
  for keyword in keywords:
    if keyword in host:
      ctx.log('[Script] matched {{ rule.group }} DOMAIN-KEYWORD rule')
      return "{{ rule.group }}")";

std::string findFileName(const std::string &path)
{
    string_size pos = path.rfind('/');
    if(pos == std::string::npos)
    {
        pos = path.rfind('\\');
        if(pos == std::string::npos)
            pos = 0;
    }
    string_size pos2 = path.rfind('.');
    if(pos2 < pos || pos2 == std::string::npos)
        pos2 = path.size();
    return path.substr(pos + 1, pos2 - pos - 1);
}

int renderClashScript(YAML::Node &base_rule, std::vector<RulesetContent> &ruleset_content_array, const std::string &remote_path_prefix, bool script, bool overwrite_original_rules, bool clash_classical_ruleset)
{
    nlohmann::json data;
    std::string match_group, geoips, retrieved_rules;
    std::string strLine, rule_group, rule_path, rule_path_typed, rule_name, old_rule_name;
    std::stringstream strStrm;
    string_array vArray, groups;
    string_map keywords, urls, names;
    std::map<std::string, bool> has_domain, has_ipcidr;
    std::map<std::string, int> ruleset_interval, rule_type;
    string_array rules;
    int index = 0;

    if(!overwrite_original_rules && base_rule["rules"].IsDefined())
        rules = safe_as<string_array>(base_rule["rules"]);

    for(RulesetContent &x : ruleset_content_array)
    {
        rule_group = x.rule_group;
        rule_path = x.rule_path;
        rule_path_typed = x.rule_path_typed;
        if(rule_path.empty())
        {
            strLine = x.rule_content.get().substr(2);
            if(script)
            {
                if(startsWith(strLine, "MATCH") || startsWith(strLine, "FINAL"))
                    match_group = rule_group;
                else if(startsWith(strLine, "GEOIP"))
                {
                    vArray = split(strLine, ",");
                    if(vArray.size() < 2)
                        continue;
                    geoips += "\"" + vArray[1] + "\": \"" + rule_group + "\",";
                }
                continue;
            }
            if(startsWith(strLine, "FINAL"))
                strLine.replace(0, 5, "MATCH");
            strLine += "," + rule_group;
            if(count_least(strLine, ',', 3))
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            rules.emplace_back(std::move(strLine));
            continue;
        }
        else
        {
            if(x.rule_type == RULESET_CLASH_IPCIDR || x.rule_type == RULESET_CLASH_DOMAIN || x.rule_type == RULESET_CLASH_CLASSICAL)
            {
                //rule_name = std::to_string(hash_(rule_group + rule_path));
                rule_name = old_rule_name = findFileName(rule_path);
                int idx = 2;
                while(std::find(groups.begin(), groups.end(), rule_name) != groups.end())
                    rule_name = old_rule_name + "_" + std::to_string(idx++);
                names[rule_name] = rule_group;
                urls[rule_name] = "*" + rule_path;
                rule_type[rule_name] = x.rule_type;
                ruleset_interval[rule_name] = x.update_interval;
                switch(x.rule_type)
                {
                case RULESET_CLASH_IPCIDR:
                    has_ipcidr[rule_name] = true;
                    break;
                case RULESET_CLASH_DOMAIN:
                    has_domain[rule_name] = true;
                    break;
                case RULESET_CLASH_CLASSICAL:
                    break;
                }
                if(!script)
                    rules.emplace_back("RULE-SET," + rule_name + "," + rule_group);
                groups.emplace_back(rule_name);
                continue;
            }
            if(!remote_path_prefix.empty())
            {
                if(fileExist(rule_path, true) || isLink(rule_path))
                {
                    //rule_name = std::to_string(hash_(rule_group + rule_path));
                    rule_name = old_rule_name = findFileName(rule_path);
                    int idx = 2;
                    while(std::find(groups.begin(), groups.end(), rule_name) != groups.end())
                        rule_name = old_rule_name + "_" + std::to_string(idx++);
                    names[rule_name] = rule_group;
                    urls[rule_name] = rule_path_typed;
                    rule_type[rule_name] = x.rule_type;
                    ruleset_interval[rule_name] = x.update_interval;
                    if(clash_classical_ruleset)
                    {
                        if(!script)
                            rules.emplace_back("RULE-SET," + rule_name + "," + rule_group);
                        groups.emplace_back(rule_name);
                        continue;
                    }
                }
                else
                    continue;
            }

            retrieved_rules = x.rule_content.get();
            if(retrieved_rules.empty())
            {
                writeLog(0, "Failed to fetch ruleset or ruleset is empty: '" + x.rule_path + "'!", LOG_LEVEL_WARNING);
                continue;
            }

            retrieved_rules = convertRuleset(retrieved_rules, x.rule_type);
            char delimiter = getLineBreak(retrieved_rules);

            strStrm.clear();
            strStrm<<retrieved_rules;
            std::string::size_type lineSize;
            bool has_no_resolve = false;
            while(getline(strStrm, strLine, delimiter))
            {
                lineSize = strLine.size();
                if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
                    strLine.erase(--lineSize);
                if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                    continue;

                if(startsWith(strLine, "DOMAIN-KEYWORD,"))
                {
                    if(script)
                    {
                        vArray = split(strLine, ",");
                        if(vArray.size() < 2)
                            continue;
                        if(keywords.find(rule_name) == keywords.end())
                            keywords[rule_name] = "\"" + vArray[1] + "\"";
                        else
                            keywords[rule_name] += ",\"" + vArray[1] + "\"";
                    }
                    else
                    {
                        vArray = split(strLine, ",");
                        if(vArray.size() < 2)
                        {
                            strLine = vArray[0] + "," + rule_group;
                        }
                        else
                        {
                            strLine = vArray[0] + "," + vArray[1] + "," + rule_group;
                            if(vArray.size() > 2)
                                strLine += "," + vArray[2];
                        }
                        rules.emplace_back(strLine);
                    }
                }
                else if(!has_domain[rule_name] && (startsWith(strLine, "DOMAIN,") || startsWith(strLine, "DOMAIN-SUFFIX,")))
                    has_domain[rule_name] = true;
                else if(!has_ipcidr[rule_name] && (startsWith(strLine, "IP-CIDR,") || startsWith(strLine, "IP-CIDR6,")))
                {
                    has_ipcidr[rule_name] = true;
                    if(strLine.find(",no-resolve") != std::string::npos)
                        has_no_resolve = true;
                }
            }
            if(has_domain[rule_name] && !script)
                rules.emplace_back("RULE-SET," + rule_name + "_domain," + rule_group);
            if(has_ipcidr[rule_name] && !script)
            {
                if(has_no_resolve)
                    rules.emplace_back("RULE-SET," + rule_name + "_ipcidr," + rule_group + ",no-resolve");
                else
                    rules.emplace_back("RULE-SET," + rule_name + "_ipcidr," + rule_group);
            }
            if(!has_domain[rule_name] && !has_ipcidr[rule_name] && !script)
                rules.emplace_back("RULE-SET," + rule_name + "," + rule_group);
            if(std::find(groups.begin(), groups.end(), rule_name) == groups.end())
                groups.emplace_back(rule_name);
        }
    }
    for(std::string &x : groups)
    {
        std::string url = urls[x], keyword = keywords[x], name = names[x];
        bool group_has_domain = has_domain[x], group_has_ipcidr = has_ipcidr[x];
        int interval = ruleset_interval[x];

        if(group_has_domain)
        {
            std::string yaml_key = x;
            if(rule_type[x] != RULESET_CLASH_DOMAIN)
                yaml_key += "_domain";
            base_rule["rule-providers"][yaml_key]["type"] = "http";
            base_rule["rule-providers"][yaml_key]["behavior"] = "domain";
            if(url[0] == '*')
                base_rule["rule-providers"][yaml_key]["url"] = url.substr(1);
            else
                base_rule["rule-providers"][yaml_key]["url"] = remote_path_prefix + "/getruleset?type=3&url=" + urlSafeBase64Encode(url);
            base_rule["rule-providers"][yaml_key]["path"] = "./providers/rule-provider_" + yaml_key + ".yaml";
            if(interval)
                base_rule["rule-providers"][yaml_key]["interval"] = interval;
        }
        if(group_has_ipcidr)
        {
            std::string yaml_key = x;
            if(rule_type[x] != RULESET_CLASH_IPCIDR)
                yaml_key += "_ipcidr";
            base_rule["rule-providers"][yaml_key]["type"] = "http";
            base_rule["rule-providers"][yaml_key]["behavior"] = "ipcidr";
            if(url[0] == '*')
                base_rule["rule-providers"][yaml_key]["url"] = url.substr(1);
            else
                base_rule["rule-providers"][yaml_key]["url"] = remote_path_prefix + "/getruleset?type=4&url=" + urlSafeBase64Encode(url);
            base_rule["rule-providers"][yaml_key]["path"] = "./providers/rule-provider_" + yaml_key + ".yaml";
            if(interval)
                base_rule["rule-providers"][yaml_key]["interval"] = interval;
        }
        if(!group_has_domain && !group_has_ipcidr)
        {
            std::string yaml_key = x;
            base_rule["rule-providers"][yaml_key]["type"] = "http";
            base_rule["rule-providers"][yaml_key]["behavior"] = "classical";
            if(url[0] == '*')
                base_rule["rule-providers"][yaml_key]["url"] = url.substr(1);
            else
                base_rule["rule-providers"][yaml_key]["url"] = remote_path_prefix + "/getruleset?type=6&url=" + urlSafeBase64Encode(url);
            base_rule["rule-providers"][yaml_key]["path"] = "./providers/rule-provider_" + yaml_key + ".yaml";
            if(interval)
                base_rule["rule-providers"][yaml_key]["interval"] = interval;
        }
        if(script)
        {
            std::string json_path = "rules." + std::to_string(index) + ".";
            parse_json_pointer(data, json_path + "has_domain", group_has_domain ? "true" : "false");
            parse_json_pointer(data, json_path + "has_ipcidr", group_has_ipcidr ? "true" : "false");
            parse_json_pointer(data, json_path + "name", x);
            parse_json_pointer(data, json_path + "group", name);
            parse_json_pointer(data, json_path + "set", "true");
            parse_json_pointer(data, json_path + "keyword", keyword);
            parse_json_pointer(data, json_path + "original", (rule_type[x] == RULESET_CLASH_DOMAIN || rule_type[x] == RULESET_CLASH_IPCIDR) ? "true" : "false");
        }
        index++;
    }
    if(script)
    {
        if(!geoips.empty())
            parse_json_pointer(data, "geoips", geoips.erase(geoips.size() - 1));

        parse_json_pointer(data, "match_group", match_group);

        inja::Environment env;
        env.include_template("keyword_template", env.parse(clash_script_keyword_template));
        env.include_template("group_template", env.parse(clash_script_group_template));
        inja::Template tmpl = env.parse(clash_script_template);

        try
        {
            std::string output_content = env.render(tmpl, data);
            base_rule["script"]["code"] = output_content;
        }
        catch (std::exception &e)
        {
            writeLog(0, "Error when rendering: " + std::string(e.what()), LOG_TYPE_ERROR);
            return -1;
        }
    }
    else
        base_rule["rules"] = rules;
    return 0;
}
