#ifndef TEMPLATES_H_INCLUDED
#define TEMPLATES_H_INCLUDED

#include <string>
#include <map>

#include "generator/config/subexport.h"
#include "utils/string.h"

struct template_args
{
    string_map global_vars;
    string_map request_params;
    string_map local_vars;
    string_map node_list;
};

int render_template(const std::string &content, const template_args &vars, std::string &output, const std::string &include_scope = "templates");
int renderClashScript(YAML::Node &base_rule, std::vector<RulesetContent> &ruleset_content_array, const std::string &remote_path_prefix, bool script, bool overwrite_original_rules, bool clash_classic_ruleset);

#endif // TEMPLATES_H_INCLUDED
