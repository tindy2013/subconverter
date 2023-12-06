#include <string>

#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/binding/nlohmann_json.h>
#include <jinja2cpp/template.h>
#include <nlohmann/json.hpp>

#include "handler/interfaces.h"
#include "utils/regexp.h"
#include "templates.h"

static inline void parse_json_pointer(nlohmann::json &json, const std::string &path, const std::string &value)
{
    std::string pointer = "/" + replaceAllDistinct(path, ".", "/");
    json[nlohmann::json::json_pointer(pointer)] = value;
}

int render_template(const std::string &content, const template_args &vars, std::string &output, const std::string &include_scope)
{
    jinja2::Template tpl;
    nlohmann::json data;
    for(auto &x : vars.global_vars)
        parse_json_pointer(data["global"], x.first, x.second);
    for(auto &x : vars.request_params)
        parse_json_pointer(data["request"], x.first, x.second);
    for(auto &x : vars.local_vars)
        parse_json_pointer(data["local"], x.first, x.second);
    tpl.Load(content);
    jinja2::ValuesMap valmap = {{"global", jinja2::Reflect(data["global"])}, {"local", jinja2::Reflect(data["local"])}, {"request", jinja2::Reflect(data["request"])}};
    valmap["fetch"] = jinja2::MakeCallable(jinja2_webGet, jinja2::ArgInfo{"url"});
    valmap["replace"] = jinja2::MakeCallable([](const std::string &src, const std::string &target, const std::string &rep)
    {
        return regReplace(src, target, rep);
    }, jinja2::ArgInfo{"src"}, jinja2::ArgInfo{"target"}, jinja2::ArgInfo{"rep"});
    try
    {
        output = tpl.RenderAsString(valmap).value();
        return 0;
    }
    catch (std::exception &e)
    {
        output = "Template render failed! Reason: " + std::string(e.what());
        return -1;
    }
    return -2;
}
