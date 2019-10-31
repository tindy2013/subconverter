#ifndef NODEINFO_H_INCLUDED
#define NODEINFO_H_INCLUDED

#include <string>

struct nodeInfo
{
    int linkType = -1;
    int id = 0;
    std::string group;
    std::string remarks;
    std::string server;
    int port = 0;
    std::string proxyStr;
};

#endif // NODEINFO_H_INCLUDED
