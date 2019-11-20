#ifndef NODEMANIP_H_INCLUDED
#define NODEMANIP_H_INCLUDED

#include <string>
#include <vector>

#include "nodeinfo.h"

void addNodes(std::string link, std::vector<nodeInfo> &allNodes, int groupID, std::string proxy = "");

#endif // NODEMANIP_H_INCLUDED
