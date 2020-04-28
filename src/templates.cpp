#include <string>
#include <map>
#include <sstream>
#include <inja.hpp>
#include <nlohmann/json.hpp>

#include "interfaces.h"
#include "templates.h"
#include "logger.h"
#include "misc.h"

static inline void parse_json_pointer(nlohmann::json &json, const std::string &path, const std::string &value)
{
    std::string pointer = "/" + replace_all_distinct(path, ".", "/");
    try
    {
        json[nlohmann::json::json_pointer(pointer)] = value;
    }
    catch (std::exception&)
    {
        //ignore broken pointer
    }
}

int render_template(const std::string &content, const template_args &vars, std::string &output, const std::string &include_scope)
{
    nlohmann::json data;
    for(auto &x : vars.global_vars)
        parse_json_pointer(data["global"], x.first, x.second);
    for(auto &x : vars.request_params)
        parse_json_pointer(data["request"], x.first, x.second);
    for(auto &x : vars.local_vars)
        parse_json_pointer(data["local"], x.first, x.second);

    inja::LexerConfig m_lexer_config;
    inja::FunctionStorage m_callbacks;
    inja::TemplateStorage m_included_templates;
    inja::ParserConfig m_parser_config;

    m_lexer_config.trim_blocks = true;
    m_lexer_config.lstrip_blocks = true;
    m_lexer_config.line_statement = "#~#";
    m_callbacks.add_callback("UrlDecode", 1, [](inja::Arguments &args)
    {
        std::string data = args.at(0)->get<std::string>();
        return UrlDecode(data);
    });
    m_callbacks.add_callback("trim_of", 2, [](inja::Arguments &args)
    {
        std::string data = args.at(0)->get<std::string>(), target = args.at(1)->get<std::string>();
        if(target.empty())
            return data;
        return trim_of(data, target[0]);
    });
    m_callbacks.add_callback("trim", 1, [](inja::Arguments &args)
    {
        std::string data = args.at(0)->get<std::string>();
        return trim(data);
    });
    m_callbacks.add_callback("find", 2, [](inja::Arguments &args)
    {
        std::string src = args.at(0)->get<std::string>(), target = args.at(1)->get<std::string>();
        return regFind(src, target);
    });
    m_callbacks.add_callback("replace", 3, [](inja::Arguments &args)
    {
        std::string src = args.at(0)->get<std::string>(), target = args.at(1)->get<std::string>(), rep = args.at(2)->get<std::string>();
        if(target.empty() || src.empty())
            return src;
        return regReplace(src, target, rep);
    });
    m_callbacks.add_callback("set", 2, [&data](inja::Arguments &args)
    {
        std::string key = args.at(0)->get<std::string>(), value = args.at(1)->get<std::string>();
        parse_json_pointer(data, key, value);
        return std::string();
    });
    m_callbacks.add_callback("split", 3, [&data](inja::Arguments &args)
    {
        std::string content = args.at(0)->get<std::string>(), delim = args.at(1)->get<std::string>(), dest = args.at(2)->get<std::string>();
        string_array vArray = split(content, delim);
        for(size_t index = 0; index < vArray.size(); index++)
            parse_json_pointer(data, dest + "." + std::to_string(index), vArray[index]);
        return std::string();
    });
    m_callbacks.add_callback("join", 2, [](inja::Arguments &args)
    {
        std::string str1 = args.at(0)->get<std::string>(), str2 = args.at(1)->get<std::string>();
        return std::move(str1) + std::move(str2);
    });
    m_callbacks.add_callback("join", 3, [](inja::Arguments &args)
    {
        std::string str1 = args.at(0)->get<std::string>(), str2 = args.at(1)->get<std::string>(), str3 = args.at(2)->get<std::string>();
        return std::move(str1) + std::move(str2) + std::move(str3);
    });
    m_callbacks.add_callback("append", 2, [&data](inja::Arguments &args)
    {
        std::string path = args.at(0)->get<std::string>(), value = args.at(1)->get<std::string>();
        std::string pointer = "/" + replace_all_distinct(path, ".", "/");
        std::string output_content;
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
        return std::string();
    });
    m_callbacks.add_callback("fetch", 1, template_webGet);
    m_callbacks.add_callback("parseHostname", 1, parseHostname);
    m_parser_config.include_scope_limit = true;
    m_parser_config.include_scope = include_scope;

    inja::Parser parser(m_parser_config, m_lexer_config, m_included_templates);
    inja::Renderer renderer(m_included_templates, m_callbacks);

    try
    {
        std::stringstream out;
        renderer.render_to(out, parser.parse(content), data);
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
{% for rule in rules %}
{% if rule.set == "true" %}{% include "group_template" %}{% endif %}
{% if not rule.keyword == "" %}{% include "keyword_template" %}{% endif %}
{% endfor %}

  ip = ctx.resolve_ip(md.host)
  if ip == "":
    return "{{ match_group }}"
  geoips = { {{ geoips }} }
  for key in geoips:
    if ctx.geoip(ip) == key:
      return geoips[key]
  return "{{ match_group }}")";

const std::string clash_script_group_template = R"(  if ctx.rule_providers["{{ rule.group }}_domain"].match(md):
    ctx.log('[Script] matched {{ rule.group }} DOMAIN rule')
    return "{{ rule.group }}"
  if ctx.rule_providers["{{ rule.group }}_ipcidr"].match(md):
    ctx.log('[Script] matched {{ rule.group }} IP rule')
    return "{{ rule.group }}")";

const std::string clash_script_keyword_template = R"(  keywords = [{{ rule.keyword }}]
  for keyword in keywords:
    if keyword in md.host:
      ctx.log('[Script] matched {{ rule.group }} DOMAIN-KEYWORD rule')
      return "{{ rule.group }}")";

int renderClashScript(YAML::Node &base_rule, std::vector<ruleset_content> &ruleset_content_array, std::string remote_path_prefix)
{
    nlohmann::json data;
    std::string match_group, geoips,  retrieved_rules;
    std::string strLine, rule_group, rule_path;
    std::stringstream strStrm;
    string_array vArray, groups;
    string_map keywords, urls;
    for(ruleset_content &x : ruleset_content_array)
    {
        rule_group = x.rule_group;
        rule_path = x.rule_path;
        if(rule_path.empty())
        {
            strLine = x.rule_content.get().substr(2);
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
        else
        {
            if(fileExist(rule_path) || startsWith(rule_path, "https://") || startsWith(rule_path, "http://") || startsWith(rule_path, "data:"))
            {
                if(urls.find(rule_group) == urls.end())
                    urls[rule_group] = rule_path;
                else
                    urls[rule_group] += "|" + rule_path;
            }
            else
                continue;

            retrieved_rules = x.rule_content.get();
            if(retrieved_rules.empty())
            {
                writeLog(0, "Failed to fetch ruleset or ruleset is empty: '" + x.rule_path + "'!", LOG_LEVEL_WARNING);
                continue;
            }

            char delimiter = std::count(retrieved_rules.begin(), retrieved_rules.end(), '\n') < 1 ? '\r' : '\n';

            strStrm.clear();
            strStrm<<retrieved_rules;
            std::string::size_type lineSize;
            while(getline(strStrm, strLine, delimiter))
            {
                lineSize = strLine.size();
                if(lineSize)
                {
                    strLine = regTrim(strLine);
                    lineSize = strLine.size();
                }
                if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                    continue;

                if(startsWith(strLine, "DOMAIN-KEYWORD,"))
                {
                    vArray = split(strLine, ",");
                    if(vArray.size() < 2)
                        continue;
                    if(keywords.find(rule_group) == keywords.end())
                        keywords[rule_group] = "\"" + vArray[1] + "\"";
                    else
                        keywords[rule_group] += ",\"" + vArray[1] + "\"";
                }
            }
            if(std::find(groups.begin(), groups.end(), rule_group) == groups.end())
                groups.emplace_back(rule_group);
        }
    }
    int index = 0;
    for(std::string &x : groups)
    {
        std::string json_path = "rules." + std::to_string(index) + ".";
        std::string url = urls[x], keyword = keywords[x];
        base_rule["rule-providers"][x + "_domain"]["type"] = "http";
        base_rule["rule-providers"][x + "_domain"]["behavior"] = "domain";
        base_rule["rule-providers"][x + "_domain"]["url"] = remote_path_prefix + "/getruleset?type=3&url=" + urlsafe_base64_encode(url);
        base_rule["rule-providers"][x + "_domain"]["path"] = "./rule-providers_" + getMD5(url + x + "domain") + ".yaml";
        base_rule["rule-providers"][x + "_ipcidr"]["type"] = "http";
        base_rule["rule-providers"][x + "_ipcidr"]["behavior"] = "ipcidr";
        base_rule["rule-providers"][x + "_ipcidr"]["url"] = remote_path_prefix + "/getruleset?type=4&url=" + urlsafe_base64_encode(url);
        base_rule["rule-providers"][x + "_ipcidr"]["path"] = "./rule-providers_" + getMD5(url + x + "ipcidr") + ".yaml";
        parse_json_pointer(data, json_path + "group", x);
        parse_json_pointer(data, json_path + "set", "true");
        parse_json_pointer(data, json_path + "keyword", keyword);
        index++;
    }
    parse_json_pointer(data, "geoips", geoips.erase(geoips.size() - 1));
    parse_json_pointer(data, "match_group", match_group);

    inja::Environment env;
    env.include_template("group_template", env.parse(clash_script_group_template));
    env.include_template("keyword_template", env.parse(clash_script_keyword_template));
    inja::Template tmpl = env.parse(clash_script_template);

    try
    {
        std::string output_content = env.render(tmpl, data);
        base_rule["script"]["code"] = output_content;
    }
    catch (std::exception&)
    {
        return -1;
    }
    return 0;
}
