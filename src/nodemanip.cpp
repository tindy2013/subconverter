#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include "nodeinfo.h"
#include "printout.h"
#include "logger.h"
#include "webget.h"
#include "speedtestutil.h"
#include "script_duktape.h"

std::string override_conf_port;
bool ss_libev, ssr_libev;
extern int gCacheSubscription;

void copyNodes(std::vector<nodeInfo> &source, std::vector<nodeInfo> &dest)
{
    std::move(source.begin(), source.end(), std::back_inserter(dest));
}

int addNodes(std::string link, std::vector<nodeInfo> &allNodes, int groupID, const std::string &proxy, string_array &exclude_remarks, string_array &include_remarks, string_array &stream_rules, string_array &time_rules, std::string &subInfo, bool authorized, string_map &request_headers)
{
    int linkType = -1;
    std::vector<nodeInfo> nodes;
    nodeInfo node;
    std::string strSub, extra_headers, custom_group;

    // TODO: replace with startsWith if appropriate
    link = replace_all_distinct(link, "\"", "");

    /// script:filepath,arg1,arg2,...
    if(startsWith(link, "script:") && authorized) /// process subscription with script
    {
        writeLog(0, "Found script link. Start running...", LOG_LEVEL_INFO);
        string_array args = split(link.substr(7), ",");
        if(args.size() >= 1)
        {
            std::string script = fileGet(args[0], false);
            duk_context *ctx = duktape_init();
            defer(duk_destroy_heap(ctx);)
            duktape_peval(ctx, script);
            duk_get_global_string(ctx, "parse");
            for(size_t i = 1; i < args.size(); i++)
                duk_push_string(ctx, trim(args[i]).c_str());
            if(duk_pcall(ctx, args.size() - 1) == 0)
                link = duktape_get_res_str(ctx);
            else
            {
                writeLog(0, "Error when trying to evaluate script:\n" + duktape_get_err_stack(ctx), LOG_LEVEL_ERROR);
                duk_pop(ctx); /// pop err
            }
        }
    }

    /// tag:group_name,link
    if(startsWith(link, "tag:"))
    {
        string_size pos = link.find(",");
        if(pos != link.npos)
        {
            custom_group = link.substr(4, pos - 4);
            link.erase(0, pos + 1);
        }
    }

    if(link == "nullnode")
    {
        node.groupID = 0;
        writeLog(0, "Adding node placeholder...");
        allNodes.emplace_back(std::move(node));
        return 0;
    }

    writeLog(LOG_TYPE_INFO, "Received Link.");
    if(startsWith(link, "https://t.me/socks") || startsWith(link, "tg://socks"))
        linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
    else if(startsWith(link, "https://t.me/http") || startsWith(link, "tg://http"))
        linkType = SPEEDTEST_MESSAGE_FOUNDHTTP;
    else if(isLink(link) || startsWith(link, "surge:///install-config"))
        linkType = SPEEDTEST_MESSAGE_FOUNDSUB;
    else if(startsWith(link, "Netch://"))
        linkType = SPEEDTEST_MESSAGE_FOUNDNETCH;
    else if(fileExist(link))
        linkType = SPEEDTEST_MESSAGE_FOUNDLOCAL;

    switch(linkType)
    {
    case SPEEDTEST_MESSAGE_FOUNDSUB:
        writeLog(LOG_TYPE_INFO, "Downloading subscription data...");
        if(startsWith(link, "surge:///install-config")) //surge config link
            link = UrlDecode(getUrlArg(link, "url"));
        strSub = webGet(link, proxy, gCacheSubscription, &extra_headers, &request_headers);
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
            if(explodeConfContent(strSub, override_conf_port, ss_libev, ssr_libev, nodes) == SPEEDTEST_ERROR_UNRECOGFILE)
            {
                writeLog(LOG_TYPE_ERROR, "Invalid subscription!");
                return -1;
            }
            if(startsWith(strSub, "ssd://"))
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
            {
                x.groupID = groupID;
                if(custom_group.size())
                    x.group = custom_group;
            }
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
        if(explodeConf(link, override_conf_port, ss_libev, ssr_libev, nodes) == SPEEDTEST_ERROR_UNRECOGFILE)
        {
            writeLog(LOG_TYPE_ERROR, "Invalid configuration file!");
            return -1;
        }
        if(startsWith(strSub, "ssd://"))
        {
            getSubInfoFromSSD(strSub, subInfo);
        }
        else
        {
            getSubInfoFromNodes(nodes, stream_rules, time_rules, subInfo);
        }
        filterNodes(nodes, exclude_remarks, include_remarks, groupID);
        for(nodeInfo &x : nodes)
        {
            x.groupID = groupID;
            if(custom_group.size())
                x.group = custom_group;
        }
        copyNodes(nodes, allNodes);
        break;
    default:
        explode(link, ss_libev, ssr_libev, override_conf_port, node);
        if(node.linkType == -1)
        {
            writeLog(LOG_TYPE_ERROR, "No valid link found.");
            return -1;
        }
        node.groupID = groupID;
        if(custom_group.size())
            node.group = custom_group;
        allNodes.emplace_back(std::move(node));
    }
    return 0;
}
