#ifndef TEMPLATES_H_INCLUDED
#define TEMPLATES_H_INCLUDED

#include <string>
#include <map>

#include "subexport.h"

typedef std::map<std::string, std::string> string_map;

struct template_args
{
    string_map global_vars;
    string_map request_params;
    string_map local_vars;
};

int render_template(const std::string &content, const template_args &vars, std::string &output, const std::string &include_scope = "template");
int renderClashScript(YAML::Node &base_rule, std::vector<ruleset_content> &ruleset_content_array, std::string remote_path_prefix, bool script, bool overwrite_original_rules, bool clash_classic_ruleset);

#endif // TEMPLATES_H_INCLUDED
