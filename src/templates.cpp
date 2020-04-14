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
    json[nlohmann::json::json_pointer(pointer)] = value;
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
