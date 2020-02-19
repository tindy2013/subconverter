#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include "nodeinfo.h"
#include "printout.h"
#include "logger.h"
#include "webget.h"
#include "speedtestutil.h"

std::string override_conf_port;
int socksport;
bool ss_libev, ssr_libev;
extern int cache_subscription;

void copyNodes(std::vector<nodeInfo> &source, std::vector<nodeInfo> &dest)
{
    for(auto &x : source)
    {
        dest.emplace_back(x);
    }
}

int addNodes(std::string link, std::vector<nodeInfo> &allNodes, int groupID, std::string proxy, string_array &exclude_remarks, string_array &include_remarks, string_array &stream_rules, string_array &time_rules, std::string &subInfo, bool authorized)
{
    int linkType = -1;
    std::vector<nodeInfo> nodes;
    nodeInfo node;
    std::string strSub, extra_headers;

    // TODO: replace with startsWith if appropriate
    link = replace_all_distinct(link, "\"", "");
    writeLog(LOG_TYPE_INFO, "Received Link.");
    if(strFind(link, "https://t.me/socks") || strFind(link, "tg://socks"))
        linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
    else if(strFind(link, "https://t.me/http") || strFind(link, "tg://http"))
        linkType = SPEEDTEST_MESSAGE_FOUNDHTTP;
    else if(startsWith(link, "http://") || startsWith(link, "https://") || startsWith(link, "data:") || strFind(link, "surge:///install-config"))
        linkType = SPEEDTEST_MESSAGE_FOUNDSUB;
    else if(strFind(link, "Netch://"))
        linkType = SPEEDTEST_MESSAGE_FOUNDNETCH;
    else if(fileExist(link))
        linkType = SPEEDTEST_MESSAGE_FOUNDLOCAL;

    switch(linkType)
    {
    case SPEEDTEST_MESSAGE_FOUNDSUB:
        writeLog(LOG_TYPE_INFO, "Downloading subscription data...");
        if(strFind(link, "surge:///install-config")) //surge config link
            link = UrlDecode(getUrlArg(link, "url"));
        strSub = webGet(link, proxy, extra_headers, cache_subscription);
        /*
        if(strSub.size() == 0)
        {
            //try to get it again with system proxy
            writeLog(LOG_TYPE_WARN, "Cannot download subscription directly. Using system proxy.");
            strProxy = getSystemProxy();
            if(strProxy != "")
            {
                strSub = webGet(link, strProxy);
            }
            else
                writeLog(LOG_TYPE_WARN, "No system proxy is set. Skipping.");
        }
        */
        if(strSub.size())
        {
            writeLog(LOG_TYPE_INFO, "Parsing subscription data...");
            if(explodeConfContent(strSub, override_conf_port, socksport, ss_libev, ssr_libev, nodes) == SPEEDTEST_ERROR_UNRECOGFILE)
            {
                writeLog(LOG_TYPE_ERROR, "Invalid subscription!");
                return -1;
            }
            if(strSub.find("ssd://") == 0)
            {
                getSubInfoFromSSD(strSub, subInfo);
            }
            else
            {
                if(!getSubInfoFromHeader(extra_headers, subInfo))
                    getSubInfoFromNodes(nodes, stream_rules, time_rules, subInfo);
            }
            filterNodes(nodes, exclude_remarks, include_remarks, groupID);
            for(nodeInfo &x : nodes)
                x.groupID = groupID;
            copyNodes(nodes, allNodes);
        }
        else
        {
            writeLog(LOG_TYPE_ERROR, "Cannot download subscription data.");
            return -1;
        }
        break;
    case SPEEDTEST_MESSAGE_FOUNDLOCAL:
        if(!authorized)
            return -1;
        writeLog(LOG_TYPE_INFO, "Parsing configuration file data...");
        if(explodeConf(link, override_conf_port, socksport, ss_libev, ssr_libev, nodes) == SPEEDTEST_ERROR_UNRECOGFILE)
        {
            writeLog(LOG_TYPE_ERROR, "Invalid configuration file!");
            return -1;
        }
        if(strSub.find("ssd://") == 0)
        {
            getSubInfoFromSSD(strSub, subInfo);
        }
        else
        {
            getSubInfoFromNodes(nodes, stream_rules, time_rules, subInfo);
        }
        filterNodes(nodes, exclude_remarks, include_remarks, groupID);
        for(nodeInfo &x : nodes)
            x.groupID = groupID;
        copyNodes(nodes, allNodes);
        break;
    default:
        explode(link, ss_libev, ssr_libev, override_conf_port, socksport, node);
        if(node.linkType == -1)
        {
            writeLog(LOG_TYPE_ERROR, "No valid link found.");
            return -1;
        }
        node.groupID = groupID;
        allNodes.push_back(node);
    }
    return 0;
}
