#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include "handler/settings.h"
#include "handler/webget.h"
#include "parser/config/proxy.h"
#include "parser/infoparser.h"
#include "parser/subparser.h"
#include "script/script_quickjs.h"
#include "utils/file_extra.h"
#include "utils/logger.h"
#include "utils/map_extra.h"
#include "utils/network.h"
#include "utils/regexp.h"
#include "utils/urlencode.h"
#include "nodemanip.h"
#include "subexport.h"

extern Settings global;

bool applyMatcher(const std::string &rule, std::string &real_rule, const Proxy &node);

int explodeConf(const std::string &filepath, std::vector<Proxy> &nodes)
{
    return explodeConfContent(fileGet(filepath), nodes);
}

void copyNodes(std::vector<Proxy> &source, std::vector<Proxy> &dest)
{
    std::move(source.begin(), source.end(), std::back_inserter(dest));
}

int addNodes(std::string link, std::vector<Proxy> &allNodes, int groupID, parse_settings &parse_set)
{
    std::string &proxy = *parse_set.proxy, &subInfo = *parse_set.sub_info;
    string_array &exclude_remarks = *parse_set.exclude_remarks;
    string_array &include_remarks = *parse_set.include_remarks;
    RegexMatchConfigs &stream_rules = *parse_set.stream_rules;
    RegexMatchConfigs &time_rules = *parse_set.time_rules;
    string_icase_map *request_headers = parse_set.request_header;
    bool &authorized = parse_set.authorized;

    ConfType linkType = ConfType::Unknow;
    std::vector<Proxy> nodes;
    Proxy node;
    std::string strSub, extra_headers, custom_group;

    // TODO: replace with startsWith if appropriate
    link = replaceAllDistinct(link, "\"", "");

    /// script:filepath,arg1,arg2,...
    if(authorized) script_safe_runner(parse_set.js_runtime, parse_set.js_context, [&](qjs::Context &ctx)
    {
        if(startsWith(link, "script:")) /// process subscription with script
        {
            writeLog(0, "Found script link. Start running...", LOG_LEVEL_INFO);
            string_array args = split(link.substr(7), ",");
            if(args.size() >= 1)
            {
                std::string script = fileGet(args[0], false);
                try
                {
                    ctx.eval(script);
                    args.erase(args.begin()); /// remove script path
                    auto parse = (std::function<std::string(const std::string&, const string_array&)>) ctx.eval("parse");
                    switch(args.size())
                    {
                    case 0:
                        link = parse("", string_array());
                        break;
                    case 1:
                        link = parse(args[0], string_array());
                        break;
                    default:
                        {
                            std::string first = args[0];
                            args.erase(args.begin());
                            link = parse(first, args);
                            break;
                        }
                    }
                }
                catch(qjs::exception)
                {
                    script_print_stack(ctx);
                }
            }
        }
    }, global.scriptCleanContext);
            /*
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
            */

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
        node.GroupId = 0;
        writeLog(0, "Adding node placeholder...");
        allNodes.emplace_back(std::move(node));
        return 0;
    }

    writeLog(LOG_TYPE_INFO, "Received Link.");
    if(startsWith(link, "https://t.me/socks") || startsWith(link, "tg://socks"))
        linkType = ConfType::SOCKS;
    else if(startsWith(link, "https://t.me/http") || startsWith(link, "tg://http"))
        linkType = ConfType::HTTP;
    else if(isLink(link) || startsWith(link, "surge:///install-config"))
        linkType = ConfType::SUB;
    else if(startsWith(link, "Netch://"))
        linkType = ConfType::Netch;
    else if(fileExist(link))
        linkType = ConfType::Local;

    switch(linkType)
    {
    case ConfType::SUB:
        writeLog(LOG_TYPE_INFO, "Downloading subscription data...");
        if(startsWith(link, "surge:///install-config")) //surge config link
            link = urlDecode(getUrlArg(link, "url"));
        strSub = webGet(link, proxy, global.cacheSubscription, &extra_headers, request_headers);
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
        if(!strSub.empty())
        {
            writeLog(LOG_TYPE_INFO, "Parsing subscription data...");
            if(explodeConfContent(strSub, nodes) == 0)
            {
                writeLog(LOG_TYPE_ERROR, "Invalid subscription: '" + link + "'!");
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
            for(Proxy &x : nodes)
            {
                x.GroupId = groupID;
                if(custom_group.size())
                    x.Group = custom_group;
            }
            copyNodes(nodes, allNodes);
        }
        else
        {
            writeLog(LOG_TYPE_ERROR, "Cannot download subscription data.");
            return -1;
        }
        break;
    case ConfType::Local:
        if(!authorized)
            return -1;
        writeLog(LOG_TYPE_INFO, "Parsing configuration file data...");
        if(explodeConf(link, nodes) == 0)
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
        for(Proxy &x : nodes)
        {
            x.GroupId = groupID;
            if(!custom_group.empty())
                x.Group = custom_group;
        }
        copyNodes(nodes, allNodes);
        break;
    default:
        explode(link, node);
        if(node.Type == ProxyType::Unknown)
        {
            writeLog(LOG_TYPE_ERROR, "No valid link found.");
            return -1;
        }
        node.GroupId = groupID;
        if(!custom_group.empty())
            node.Group = custom_group;
        allNodes.emplace_back(std::move(node));
    }
    return 0;
}

bool chkIgnore(const Proxy &node, string_array &exclude_remarks, string_array &include_remarks)
{
    bool excluded = false, included = false;
    //std::string remarks = UTF8ToACP(node.remarks);
    //std::string remarks = node.remarks;
    //writeLog(LOG_TYPE_INFO, "Comparing exclude remarks...");
    excluded = std::any_of(exclude_remarks.cbegin(), exclude_remarks.cend(), [&node](const auto &x)
    {
        std::string real_rule;
        if(applyMatcher(x, real_rule, node))
        {
            if(real_rule.empty()) return true;
            return regFind(node.Remark, real_rule);
        }
        else
            return false;
    });
    if(include_remarks.size() != 0)
    {
        //writeLog(LOG_TYPE_INFO, "Comparing include remarks...");
        included = std::any_of(include_remarks.cbegin(), include_remarks.cend(), [&node](const auto &x)
        {
            std::string real_rule;
            if(applyMatcher(x, real_rule, node))
            {
                if(real_rule.empty()) return true;
                return regFind(node.Remark, real_rule);
            }
            else
                return false;
        });
    }
    else
    {
        included = true;
    }

    return excluded || !included;
}

void filterNodes(std::vector<Proxy> &nodes, string_array &exclude_remarks, string_array &include_remarks, int groupID)
{
    int node_index = 0;
    std::vector<Proxy>::iterator iter = nodes.begin();
    while(iter != nodes.end())
    {
        if(chkIgnore(*iter, exclude_remarks, include_remarks))
        {
            writeLog(LOG_TYPE_INFO, "Node  " + iter->Group + " - " + iter->Remark + "  has been ignored and will not be added.");
            nodes.erase(iter);
        }
        else
        {
            writeLog(LOG_TYPE_INFO, "Node  " + iter->Group + " - " + iter->Remark + "  has been added.");
            iter->Id = node_index;
            iter->GroupId = groupID;
            ++node_index;
            ++iter;
        }
    }
    /*
    std::vector<std::unique_ptr<pcre2_code, decltype(&pcre2_code_free)>> exclude_patterns, include_patterns;
    std::vector<std::unique_ptr<pcre2_match_data, decltype(&pcre2_match_data_free)>> exclude_match_data, include_match_data;
    unsigned int i = 0;
    PCRE2_SIZE erroroffset;
    int errornumber, rc;

    for(i = 0; i < exclude_remarks.size(); i++)
    {
        std::unique_ptr<pcre2_code, decltype(&pcre2_code_free)> pattern(pcre2_compile(reinterpret_cast<const unsigned char*>(exclude_remarks[i].c_str()), exclude_remarks[i].size(), PCRE2_UTF | PCRE2_MULTILINE | PCRE2_ALT_BSUX, &errornumber, &erroroffset, NULL), &pcre2_code_free);
        if(!pattern)
            return;
        exclude_patterns.emplace_back(std::move(pattern));
        pcre2_jit_compile(exclude_patterns[i].get(), 0);
        std::unique_ptr<pcre2_match_data, decltype(&pcre2_match_data_free)> match_data(pcre2_match_data_create_from_pattern(exclude_patterns[i].get(), NULL), &pcre2_match_data_free);
        exclude_match_data.emplace_back(std::move(match_data));
    }
    for(i = 0; i < include_remarks.size(); i++)
    {
        std::unique_ptr<pcre2_code, decltype(&pcre2_code_free)> pattern(pcre2_compile(reinterpret_cast<const unsigned char*>(include_remarks[i].c_str()), include_remarks[i].size(), PCRE2_UTF | PCRE2_MULTILINE | PCRE2_ALT_BSUX, &errornumber, &erroroffset, NULL), &pcre2_code_free);
        if(!pattern)
            return;
        include_patterns.emplace_back(std::move(pattern));
        pcre2_jit_compile(include_patterns[i].get(), 0);
        std::unique_ptr<pcre2_match_data, decltype(&pcre2_match_data_free)> match_data(pcre2_match_data_create_from_pattern(include_patterns[i].get(), NULL), &pcre2_match_data_free);
        include_match_data.emplace_back(std::move(match_data));
    }
    writeLog(LOG_TYPE_INFO, "Filter started.");
    while(iter != nodes.end())
    {
        bool excluded = false, included = false;
        for(i = 0; i < exclude_patterns.size(); i++)
        {
            rc = pcre2_match(exclude_patterns[i].get(), reinterpret_cast<const unsigned char*>(iter->remarks.c_str()), iter->remarks.size(), 0, 0, exclude_match_data[i].get(), NULL);
            if (rc < 0)
            {
                switch(rc)
                {
                case PCRE2_ERROR_NOMATCH:
                    break;
                default:
                    return;
                }
            }
            else
                excluded = true;
        }
        if(include_patterns.size() > 0)
            for(i = 0; i < include_patterns.size(); i++)
            {
                rc = pcre2_match(include_patterns[i].get(), reinterpret_cast<const unsigned char*>(iter->remarks.c_str()), iter->remarks.size(), 0, 0, include_match_data[i].get(), NULL);
                if (rc < 0)
                {
                    switch(rc)
                    {
                    case PCRE2_ERROR_NOMATCH:
                        break;
                    default:
                        return;
                    }
                }
                else
                    included = true;
            }
        else
            included = true;
        if(excluded || !included)
        {
            writeLog(LOG_TYPE_INFO, "Node  " + iter->group + " - " + iter->remarks + "  has been ignored and will not be added.");
            nodes.erase(iter);
        }
        else
        {
            writeLog(LOG_TYPE_INFO, "Node  " + iter->group + " - " + iter->remarks + "  has been added.");
            iter->id = node_index;
            iter->groupID = groupID;
            ++node_index;
            ++iter;
        }
    }
    */
    writeLog(LOG_TYPE_INFO, "Filter done.");
}

void nodeRename(Proxy &node, const RegexMatchConfigs &rename_array, extra_settings &ext)
{
    std::string &remark = node.Remark, original_remark = node.Remark, returned_remark, real_rule;

    for(const RegexMatchConfig &x : rename_array)
    {
        if(!x.Script.empty() && ext.authorized)
        {
            script_safe_runner(ext.js_runtime, ext.js_context, [&](qjs::Context &ctx)
            {
                std::string script = x.Script;
                if(startsWith(script, "path:"))
                    script = fileGet(script.substr(5), true);
                try
                {
                    ctx.eval(script);
                    auto rename = (std::function<std::string(const Proxy&)>) ctx.eval("rename");
                    returned_remark = rename(node);
                    if(!returned_remark.empty())
                        remark = returned_remark;
                }
                catch (qjs::exception)
                {
                    script_print_stack(ctx);
                }
            }, global.scriptCleanContext);
            continue;
        }
        if(applyMatcher(x.Match, real_rule, node) && real_rule.size())
            remark = regReplace(remark, real_rule, x.Replace);
    }
    if(remark.empty())
        remark = original_remark;
    return;
}

std::string removeEmoji(const std::string &orig_remark)
{
    char emoji_id[2] = {(char)-16, (char)-97};
    std::string remark = orig_remark;
    while(true)
    {
        if(remark[0] == emoji_id[0] && remark[1] == emoji_id[1])
            remark.erase(0, 4);
        else
            break;
    }
    if(remark.empty())
        return orig_remark;
    return remark;
}

std::string addEmoji(const Proxy &node, const RegexMatchConfigs &emoji_array, extra_settings &ext)
{
    std::string real_rule, ret;

    for(const RegexMatchConfig &x : emoji_array)
    {
        if(!x.Script.empty() && ext.authorized)
        {
            std::string result;
            script_safe_runner(ext.js_runtime, ext.js_context, [&](qjs::Context &ctx)
            {
                std::string script = x.Script;
                if(startsWith(script, "path:"))
                    script = fileGet(script.substr(5), true);
                try
                {
                    ctx.eval(script);
                    auto getEmoji = (std::function<std::string(const Proxy&)>) ctx.eval("getEmoji");
                    ret = getEmoji(node);
                    if(!ret.empty())
                        result = ret + " " + node.Remark;
                }
                catch (qjs::exception)
                {
                    script_print_stack(ctx);
                }
            }, global.scriptCleanContext);
            if(!result.empty())
                return result;
            continue;
        }
        if(x.Replace.empty())
            continue;
        if(applyMatcher(x.Match, real_rule, node) && real_rule.size() && regFind(node.Remark, real_rule))
            return x.Replace + " " + node.Remark;
    }
    return node.Remark;
}

void preprocessNodes(std::vector<Proxy> &nodes, extra_settings &ext)
{
    std::for_each(nodes.begin(), nodes.end(), [&ext](Proxy &x)
    {
        if(ext.remove_emoji)
            x.Remark = trim(removeEmoji(x.Remark));

        nodeRename(x, ext.rename_array, ext);

        if(ext.add_emoji)
            x.Remark = addEmoji(x, ext.emoji_array, ext);
    });

    if(ext.sort_flag)
    {
        bool failed = true;
        if(ext.sort_script.size() && ext.authorized)
        {
            std::string script = ext.sort_script;
            if(startsWith(script, "path:"))
                script = fileGet(script.substr(5), false);
            script_safe_runner(ext.js_runtime, ext.js_context, [&](qjs::Context &ctx)
            {
                try
                {
                    ctx.eval(script);
                    auto compare = (std::function<int(const Proxy&, const Proxy&)>) ctx.eval("compare");
                    auto comparer = [&](const Proxy &a, const Proxy &b)
                    {
                        if(a.Type == ProxyType::Unknown)
                            return 1;
                        if(b.Type == ProxyType::Unknown)
                            return 0;
                        return compare(a, b);
                    };
                    std::stable_sort(nodes.begin(), nodes.end(), comparer);
                    failed = false;
                }
                catch(qjs::exception)
                {
                    script_print_stack(ctx);
                }
            }, global.scriptCleanContext);
        }
        if(failed) std::stable_sort(nodes.begin(), nodes.end(), [](const Proxy &a, const Proxy &b)
        {
            return a.Remark < b.Remark;
        });
    }
}
