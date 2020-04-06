#include <string>
#include <map>
#include <sstream>
#include <inja.hpp>
#include <nlohmann/json.hpp>

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
