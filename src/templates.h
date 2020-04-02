#ifndef TEMPLATES_H_INCLUDED
#define TEMPLATES_H_INCLUDED

#include <string>
#include <map>

typedef std::map<std::string, std::string> string_map;

struct template_args
{
    string_map global_vars;
    string_map request_params;
    string_map local_vars;
};

int render_template(const std::string &content, const template_args &vars, std::string &output, const std::string &include_scope = "template");

#endif // TEMPLATES_H_INCLUDED
