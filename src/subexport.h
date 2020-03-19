#ifndef SUBEXPORT_H_INCLUDED
#define SUBEXPORT_H_INCLUDED

#include <string>
#include <vector>
#include <future>

#include "misc.h"
#include "ini_reader.h"
#include "nodeinfo.h"

struct ruleset_content
{
    std::string rule_group;
    std::string rule_path;
    std::shared_future<std::string> rule_content;
};

struct extra_settings
{
    bool enable_rule_generator = true;
    bool overwrite_original_rules = true;
    string_array rename_array;
    string_array emoji_array;
    bool add_emoji = false;
    bool remove_emoji = false;
    bool append_proxy_type = false;
    bool udp = false;
    bool tfo = false;
    bool nodelist = false;
    bool sort_flag = false;
    bool skip_cert_verify = false;
    bool filter_deprecated = false;
    bool clash_new_field_name = false;
    std::string surge_ssr_path;
    std::string managed_config_prefix;
    std::string quanx_dev_id;
};

void rulesetToClash(YAML::Node &base_rule, std::vector<ruleset_content> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name);
void rulesetToSurge(INIReader &base_rule, std::vector<ruleset_content> &ruleset_content_array, int surge_ver, bool overwrite_original_rules, std::string remote_path_prefix);
void preprocessNodes(std::vector<nodeInfo> &nodes, extra_settings &ext);

std::string netchToClash(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, bool clashR, extra_settings &ext);
void netchToClash(std::vector<nodeInfo> &nodes, YAML::Node &yamlnode, string_array &extra_proxy_group, bool clashR, extra_settings &ext);
std::string netchToSurge(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, int surge_ver, extra_settings &ext);
std::string netchToMellow(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext);
void netchToMellow(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext);
std::string netchToLoon(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext);
std::string netchToSS(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToSSSub(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToSSR(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToVMess(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToTrojan(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToQuanX(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext);
void netchToQuanX(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext);
std::string netchToQuan(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext);
void netchToQuan(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext);
std::string netchToSSD(std::vector<nodeInfo> &nodes, std::string &group, std::string &userinfo, extra_settings &ext);

std::string buildGistData(std::string name, std::string content);
int uploadGist(std::string name, std::string path, std::string content, bool writeManageURL);

#endif // SUBEXPORT_H_INCLUDED
