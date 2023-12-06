#ifndef RULECONVERT_H_INCLUDED
#define RULECONVERT_H_INCLUDED

#include <string>
#include <vector>
#include <future>

#include <yaml-cpp/yaml.h>
#include <rapidjson/document.h>

#include "utils/ini_reader/ini_reader.h"

enum ruleset_type
{
    RULESET_SURGE,
    RULESET_QUANX,
    RULESET_CLASH_DOMAIN,
    RULESET_CLASH_IPCIDR,
    RULESET_CLASH_CLASSICAL
};

struct RulesetContent
{
    std::string rule_group;
    std::string rule_path;
    std::string rule_path_typed;
    int rule_type = RULESET_SURGE;
    std::shared_future<std::string> rule_content;
    int update_interval = 0;
};

std::string convertRuleset(const std::string &content, int type);
void rulesetToClash(YAML::Node &base_rule, std::vector<RulesetContent> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name);
std::string rulesetToClashStr(YAML::Node &base_rule, std::vector<RulesetContent> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name);
void rulesetToSurge(INIReader &base_rule, std::vector<RulesetContent> &ruleset_content_array, int surge_ver, bool overwrite_original_rules, const std::string& remote_path_prefix);
void rulesetToSingBox(rapidjson::Document &base_rule, std::vector<RulesetContent> &ruleset_content_array, bool overwrite_original_rules);

#endif // RULECONVERT_H_INCLUDED
