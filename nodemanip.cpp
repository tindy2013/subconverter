#include <string>
#include <vector>
#include <iostream>

#include "nodeinfo.h"
#include "printout.h"
#include "logger.h"
#include "webget.h"
#include "speedtestutil.h"

std::string override_conf_port, custom_group;
int socksport;
bool ss_libev, ssr_libev;

void copyNodes(std::vector<nodeInfo> *source, std::vector<nodeInfo> *dest)
{
    for(auto &x : *source)
    {
        dest->push_back(x);
    }
}

void addNodes(std::string link, std::vector<nodeInfo> &allNodes)
{
    int linkType = -1;
    std::vector<nodeInfo> nodes;
    nodeInfo node;
    std::string strSub, strInput, fileContent, strProxy;

    link = replace_all_distinct(link, "\"", "");
    writeLog(LOG_TYPE_INFO, "Received Link.");
    if(strFind(link, "vmess://") || strFind(link, "vmess1://"))
        linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
    else if(strFind(link, "ss://"))
        linkType = SPEEDTEST_MESSAGE_FOUNDSS;
    else if(strFind(link, "ssr://"))
        linkType = SPEEDTEST_MESSAGE_FOUNDSSR;
    else if(strFind(link, "socks://") || strFind(link, "https://t.me/socks") || strFind(link, "tg://socks"))
        linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
    else if(strFind(link, "http://") || strFind(link, "https://") || strFind(link, "surge:///install-config"))
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
        strSub = webGet(link);
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
        if(strSub.size())
        {
            writeLog(LOG_TYPE_INFO, "Parsing subscription data...");
            explodeConfContent(strSub, override_conf_port, socksport, ss_libev, ssr_libev, nodes);
            copyNodes(&nodes, &allNodes);
        }
        else
        {
            writeLog(LOG_TYPE_ERROR, "Cannot download subscription data.");
        }
        break;
    case SPEEDTEST_MESSAGE_FOUNDLOCAL:
        writeLog(LOG_TYPE_INFO, "Parsing configuration file data...");
        if(explodeConf(link, override_conf_port, socksport, ss_libev, ssr_libev, nodes) == SPEEDTEST_ERROR_UNRECOGFILE)
        {
            writeLog(LOG_TYPE_ERROR, "Invalid configuration file!");
        }
        else
        {
            copyNodes(&nodes, &allNodes);
        }
        break;
    default:
        if(linkType > 0)
        {
            explode(link, ss_libev, ssr_libev, override_conf_port, socksport, node);
            if(custom_group.size() != 0)
                node.group = custom_group;
            if(node.server == "")
            {
                writeLog(LOG_TYPE_ERROR, "No valid link found.");
            }
            else
            {
                allNodes.push_back(node);
            }
        }
        else
        {
            writeLog(LOG_TYPE_ERROR, "No valid link found.");
        }
    }
}
