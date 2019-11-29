#ifndef SUBEXPORT_H_INCLUDED
#define SUBEXPORT_H_INCLUDED

#include <string>

struct ruleset_content
{
    std::string rule_group;
    std::string rule_path;
    std::string rule_content;
};

struct extra_settings
{
    bool add_emoji = false;
    bool remove_emoji = false;
    bool append_proxy_type = true;
    bool udp = false;
    bool tfo = false;
    bool nodelist = false;
};


std::string netchToClash(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, bool clashR, extra_settings &ext);
std::string netchToSurge(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, int surge_ver, extra_settings &ext);
std::string netchToMellow(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext);
std::string netchToSS(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToSSR(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToVMess(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToQuanX(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToQuan(std::vector<nodeInfo> &nodes, extra_settings &ext);
std::string netchToSSD(std::vector<nodeInfo> &nodes, std::string &group, extra_settings &ext);
std::string buildGistData(std::string name, std::string content);
int uploadGist(std::string name, std::string path, std::string content, bool writeManageURL);

#endif // SUBEXPORT_H_INCLUDED
