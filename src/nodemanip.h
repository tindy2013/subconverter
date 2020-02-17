#ifndef NODEMANIP_H_INCLUDED
#define NODEMANIP_H_INCLUDED

#include <string>
#include <vector>

#include "nodeinfo.h"

int addNodes(std::string link, std::vector<nodeInfo> &allNodes, int groupID, std::string proxy, string_array &exclude_remarks, string_array &include_remarks, string_array &stream_rules, string_array &time_rules, std::string &subInfo, bool authorized);

#endif // NODEMANIP_H_INCLUDED
