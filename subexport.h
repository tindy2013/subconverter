#ifndef SUBEXPORT_H_INCLUDED
#define SUBEXPORT_H_INCLUDED

#include <string>

struct ruleset_content
{
    std::string rule_group;
    std::string rule_content;
};


std::string netchToClash(std::vector<nodeInfo> &nodes, std::string &baseConf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, bool clashR);
std::string netchToSurge(std::vector<nodeInfo> &nodes, std::string &base_conf, string_array &ruleset_array, string_array &extra_proxy_group, int surge_ver);
std::string netchToSS(std::vector<nodeInfo> &nodes);
std::string netchToSSR(std::vector<nodeInfo> &nodes);
std::string netchToVMess(std::vector<nodeInfo> &nodes);
std::string netchToQuanX(std::vector<nodeInfo> &nodes);
std::string netchToQuan(std::vector<nodeInfo> &nodes);
std::string netchToSSD(std::vector<nodeInfo> &nodes, std::string &group);
std::string buildGistData(std::string name, std::string content);
int uploadGist(std::string path, std::string content, bool writeManageURL);

#endif // SUBEXPORT_H_INCLUDED
