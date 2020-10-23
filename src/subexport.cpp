#include <algorithm>
#include <iostream>
#include <numeric>
#include <cmath>
#include <climits>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <yaml-cpp/yaml.h>
#include <duktape.h>

#include "misc.h"
#include "speedtestutil.h"
#include "ini_reader.h"
#include "rapidjson_extra.h"
#include "webget.h"
#include "subexport.h"
#include "printout.h"
#include "multithread.h"
#include "socket.h"
#include "string_hash.h"
#include "logger.h"
#include "templates.h"
#include "script_duktape.h"
#include "yamlcpp_extra.h"
#include "interfaces.h"

extern bool gAPIMode, gSurgeResolveHostname;
extern string_array ss_ciphers, ssr_ciphers;
extern size_t gMaxAllowedRules;

const string_array clashr_protocols = {"origin", "auth_sha1_v4", "auth_aes128_md5", "auth_aes128_sha1", "auth_chain_a", "auth_chain_b"};
const string_array clashr_obfs = {"plain", "http_simple", "http_post", "random_head", "tls1.2_ticket_auth", "tls1.2_ticket_fastauth"};
const string_array clash_ssr_ciphers = {"rc4-md5", "aes-128-ctr", "aes-192-ctr", "aes-256-ctr", "aes-128-cfb", "aes-192-cfb", "aes-256-cfb", "chacha20-ietf", "xchacha20"};

/// rule type lists
#define basic_types "DOMAIN", "DOMAIN-SUFFIX", "DOMAIN-KEYWORD", "IP-CIDR", "SRC-IP-CIDR", "GEOIP", "MATCH", "FINAL"
string_array ClashRuleTypes = {basic_types, "IP-CIDR6", "SRC-PORT", "DST-PORT", "PROCESS-NAME"};
string_array Surge2RuleTypes = {basic_types, "IP-CIDR6", "USER-AGENT", "URL-REGEX", "PROCESS-NAME", "IN-PORT", "DEST-PORT", "SRC-IP"};
string_array SurgeRuleTypes = {basic_types, "IP-CIDR6", "USER-AGENT", "URL-REGEX", "AND", "OR", "NOT", "PROCESS-NAME", "IN-PORT", "DEST-PORT", "SRC-IP"};
string_array QuanXRuleTypes = {basic_types, "USER-AGENT", "HOST", "HOST-SUFFIX", "HOST-KEYWORD"};
string_array SurfRuleTypes = {basic_types, "IP-CIDR6", "PROCESS-NAME", "IN-PORT", "DEST-PORT", "SRC-IP"};

std::string hostnameToIPAddr(const std::string &host)
{
    int retVal;
    std::string retAddr;
    char cAddr[128] = {};
    struct sockaddr_in *target;
    struct sockaddr_in6 *target6;
    struct addrinfo hint = {}, *retAddrInfo, *cur;
    retVal = getaddrinfo(host.data(), NULL, &hint, &retAddrInfo);
    if(retVal != 0)
    {
        freeaddrinfo(retAddrInfo);
        return std::string();
    }

    for(cur = retAddrInfo; cur != NULL; cur = cur->ai_next)
    {
        if(cur->ai_family == AF_INET)
        {
            target = reinterpret_cast<struct sockaddr_in *>(cur->ai_addr);
            inet_ntop(AF_INET, &target->sin_addr, cAddr, sizeof(cAddr));
            break;
        }
        else if(cur->ai_family == AF_INET6)
        {
            target6 = reinterpret_cast<struct sockaddr_in6 *>(cur->ai_addr);
            inet_ntop(AF_INET6, &target6->sin6_addr, cAddr, sizeof(cAddr));
            break;
        }
    }
    retAddr.assign(cAddr);
    freeaddrinfo(retAddrInfo);
    return retAddr;
}

std::string vmessConstruct(const std::string &group, const std::string &remarks, const std::string &add, const std::string &port, const std::string &type, const std::string &id, const std::string &aid, const std::string &net, const std::string &cipher, const std::string &path, const std::string &host, const std::string &edge, const std::string &tls, tribool udp, tribool tfo, tribool scv, tribool tls13)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("VMess");
    writer.Key("Group");
    writer.String(group.data());
    writer.Key("Remark");
    writer.String(remarks.data());
    writer.Key("Hostname");
    writer.String(add.data());
    writer.Key("Port");
    writer.Int(to_int(port));
    writer.Key("UserID");
    writer.String(id.empty() ? "00000000-0000-0000-0000-000000000000" : id.data());
    writer.Key("AlterID");
    writer.Int(to_int(aid));
    writer.Key("EncryptMethod");
    writer.String(cipher.data());
    writer.Key("TransferProtocol");
    writer.String(net.empty() ? "tcp" : net.data());
    writer.Key("Host");
    writer.String(host.empty() ? add.data() : trim(host).data());
    writer.Key("Edge");
    writer.String(edge.data());
    if(net == "ws" || net == "http")
    {
        writer.Key("Path");
        writer.String(path.empty() ? "/" : trim(path).data());
    }
    else
    {
        if(net == "quic")
        {
            writer.Key("QUICSecure");
            writer.String(host.data());
            writer.Key("QUICSecret");
            writer.String(path.data());
        }
        writer.Key("FakeType");
        writer.String(type.data());
    }
    writer.Key("TLSSecure");
    writer.Bool(tls == "tls");
    if(!udp.is_undef())
    {
        writer.Key("EnableUDP");
        writer.Bool(udp);
    }
    if(!tfo.is_undef())
    {
        writer.Key("EnableTFO");
        writer.Bool(tfo);
    }
    if(!scv.is_undef())
    {
        writer.Key("AllowInsecure");
        writer.Bool(scv);
    }
    if(!tls13.is_undef())
    {
        writer.Key("TLS13");
        writer.Bool(tls13);
    }
    writer.EndObject();
    return sb.GetString();
}

std::string ssrConstruct(const std::string &group, const std::string &remarks, const std::string &remarks_base64, const std::string &server, const std::string &port, const std::string &protocol, const std::string &method, const std::string &obfs, const std::string &password, const std::string &obfsparam, const std::string &protoparam, bool libev, tribool udp, tribool tfo, tribool scv)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("SSR");
    writer.Key("Remark");
    writer.String(remarks.data());
    writer.Key("Hostname");
    writer.String(server.data());
    writer.Key("Port");
    writer.Int(to_int(port));
    writer.Key("Password");
    writer.String(password.data());
    writer.Key("EncryptMethod");
    writer.String(method.data());
    writer.Key("Protocol");
    writer.String(protocol.data());
    writer.Key("ProtocolParam");
    writer.String(protoparam.data());
    writer.Key("OBFS");
    writer.String(obfs.data());
    writer.Key("OBFSParam");
    writer.String(obfsparam.data());
    if(!udp.is_undef())
    {
        writer.Key("EnableUDP");
        writer.Bool(udp);
    }
    if(!tfo.is_undef())
    {
        writer.Key("EnableTFO");
        writer.Bool(tfo);
    }
    if(!scv.is_undef())
    {
        writer.Key("AllowInsecure");
        writer.Bool(scv);
    }
    writer.EndObject();
    return sb.GetString();
}

std::string ssConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &method, const std::string &plugin, const std::string &pluginopts, bool libev, tribool udp, tribool tfo, tribool scv, tribool tls13)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("SS");
    writer.Key("Group");
    writer.String(group.data());
    writer.Key("Remark");
    writer.String(remarks.data());
    writer.Key("Hostname");
    writer.String(server.data());
    writer.Key("Port");
    writer.Int(to_int(port));
    writer.Key("Password");
    writer.String(password.data());
    writer.Key("EncryptMethod");
    writer.String(method.data());
    writer.Key("Plugin");
    writer.String(plugin.data());
    writer.Key("PluginOption");
    writer.String(pluginopts.data());
    if(!udp.is_undef())
    {
        writer.Key("EnableUDP");
        writer.Bool(udp);
    }
    if(!tfo.is_undef())
    {
        writer.Key("EnableTFO");
        writer.Bool(tfo);
    }
    if(!scv.is_undef())
    {
        writer.Key("AllowInsecure");
        writer.Bool(scv);
    }
    if(!tls13.is_undef())
    {
        writer.Key("TLS13");
        writer.Bool(tls13);
    }
    writer.EndObject();
    return sb.GetString();
}

std::string socksConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &username, const std::string &password, tribool udp, tribool tfo, tribool scv)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("Socks5");
    writer.Key("Group");
    writer.String(group.data());
    writer.Key("Remark");
    writer.String(remarks.data());
    writer.Key("Hostname");
    writer.String(server.data());
    writer.Key("Port");
    writer.Int(to_int(port));
    writer.Key("Username");
    writer.String(username.data());
    writer.Key("Password");
    writer.String(password.data());
    if(!udp.is_undef())
    {
        writer.Key("EnableUDP");
        writer.Bool(udp);
    }
    if(!tfo.is_undef())
    {
        writer.Key("EnableTFO");
        writer.Bool(tfo);
    }
    if(!scv.is_undef())
    {
        writer.Key("AllowInsecure");
        writer.Bool(scv);
    }
    writer.EndObject();
    return sb.GetString();
}

std::string httpConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &username, const std::string &password, bool tls, tribool tfo, tribool scv, tribool tls13)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String(tls ? "HTTPS" : "HTTP");
    writer.Key("Group");
    writer.String(group.data());
    writer.Key("Remark");
    writer.String(remarks.data());
    writer.Key("Hostname");
    writer.String(server.data());
    writer.Key("Port");
    writer.Int(to_int(port));
    writer.Key("Username");
    writer.String(username.data());
    writer.Key("Password");
    writer.String(password.data());
    writer.Key("TLSSecure");
    writer.Bool(tls);
    if(!tfo.is_undef())
    {
        writer.Key("EnableTFO");
        writer.Bool(tfo);
    }
    if(!scv.is_undef())
    {
        writer.Key("AllowInsecure");
        writer.Bool(scv);
    }
    if(!tls13.is_undef())
    {
        writer.Key("TLS13");
        writer.Bool(tls13);
    }
    writer.EndObject();
    return sb.GetString();
}

std::string trojanConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &host, bool tlssecure, tribool udp, tribool tfo, tribool scv, tribool tls13)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("Trojan");
    writer.Key("Group");
    writer.String(group.data());
    writer.Key("Remark");
    writer.String(remarks.data());
    writer.Key("Hostname");
    writer.String(server.data());
    writer.Key("Port");
    writer.Int(to_int(port));
    writer.Key("Password");
    writer.String(password.data());
    writer.Key("Host");
    writer.String(host.data());
    writer.Key("TLSSecure");
    writer.Bool(tlssecure);
    if(!udp.is_undef())
    {
        writer.Key("EnableUDP");
        writer.Bool(udp);
    }
    if(!tfo.is_undef())
    {
        writer.Key("EnableTFO");
        writer.Bool(tfo);
    }
    if(!scv.is_undef())
    {
        writer.Key("AllowInsecure");
        writer.Bool(scv);
    }
    if(!tls13.is_undef())
    {
        writer.Key("TLS13");
        writer.Bool(tls13);
    }
    writer.EndObject();
    return sb.GetString();
}

std::string snellConstruct(const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &obfs, const std::string &host, tribool udp, tribool tfo, tribool scv)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("Snell");
    writer.Key("Group");
    writer.String(group.data());
    writer.Key("Remark");
    writer.String(remarks.data());
    writer.Key("Hostname");
    writer.String(server.data());
    writer.Key("Port");
    writer.Int(to_int(port));
    writer.Key("Password");
    writer.String(password.data());
    writer.Key("OBFS");
    writer.String(obfs.data());
    writer.Key("Host");
    writer.String(host.data());
    if(!udp.is_undef())
    {
        writer.Key("EnableUDP");
        writer.Bool(udp);
    }
    if(!tfo.is_undef())
    {
        writer.Key("EnableTFO");
        writer.Bool(tfo);
    }
    if(!scv.is_undef())
    {
        writer.Key("AllowInsecure");
        writer.Bool(scv);
    }
    writer.EndObject();
    return sb.GetString();
}

std::string vmessLinkConstruct(const std::string &remarks, const std::string &add, const std::string &port, const std::string &type, const std::string &id, const std::string &aid, const std::string &net, const std::string &path, const std::string &host, const std::string &tls)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("v");
    writer.String("2");
    writer.Key("ps");
    writer.String(remarks.data());
    writer.Key("add");
    writer.String(add.data());
    writer.Key("port");
    writer.Int(to_int(port));
    writer.Key("type");
    writer.String(type.empty() ? "none" : type.data());
    writer.Key("id");
    writer.String(id.data());
    writer.Key("aid");
    writer.Int(to_int(aid));
    writer.Key("net");
    writer.String(net.empty() ? "tcp" : net.data());
    writer.Key("path");
    writer.String(path.data());
    writer.Key("host");
    writer.String(host.data());
    writer.Key("tls");
    writer.String(tls.data());
    writer.EndObject();
    return sb.GetString();
}

bool matchRange(const std::string &range, int target)
{
    string_array vArray = split(range, ",");
    bool match = false;
    std::string range_begin_str, range_end_str;
    int range_begin, range_end;
    static const std::string reg_num = "-?\\d+", reg_range = "(\\d+)-(\\d+)", reg_not = "\\!-?(\\d+)", reg_not_range = "\\!(\\d+)-(\\d+)", reg_less = "(\\d+)-", reg_more = "(\\d+)\\+";
    for(std::string &x : vArray)
    {
        if(regMatch(x, reg_num))
        {
            if(to_int(x, INT_MAX) == target)
                match = true;
        }
        else if(regMatch(x, reg_range))
        {
            /*
            range_begin = to_int(regReplace(x, reg_range, "$1"), INT_MAX);
            range_end = to_int(regReplace(x, reg_range, "$2"), INT_MIN);
            */
            regGetMatch(x, reg_range, 3, 0, &range_begin_str, &range_end_str);
            range_begin = to_int(range_begin_str, INT_MAX);
            range_end = to_int(range_end_str, INT_MIN);
            if(target >= range_begin && target <= range_end)
                match = true;
        }
        else if(regMatch(x, reg_not))
        {
            if(to_int(regReplace(x, reg_not, "$1"), INT_MAX) == target)
                match = false;
        }
        else if(regMatch(x, reg_not_range))
        {
            /*
            range_begin = to_int(regReplace(x, reg_range, "$1"), INT_MAX);
            range_end = to_int(regReplace(x, reg_range, "$2"), INT_MIN);
            */
            regGetMatch(x, reg_range, 3, 0, &range_begin_str, &range_end_str);
            range_begin = to_int(range_begin_str, INT_MAX);
            range_end = to_int(range_end_str, INT_MIN);
            if(target >= range_begin && target <= range_end)
                match = false;
        }
        else if(regMatch(x, reg_less))
        {
            if(to_int(regReplace(x, reg_less, "$1"), INT_MAX) <= target)
                match = true;
        }
        else if(regMatch(x, reg_more))
        {
            if(to_int(regReplace(x, reg_more, "$1"), INT_MIN) >= target)
                match = true;
        }
    }
    return match;
}

bool applyMatcher(const std::string &rule, std::string &real_rule, const nodeInfo &node)
{
    std::string group, ret_real_rule;
    static const std::string groupid_regex = R"(^!!(?:GROUPID|INSERT)=([\d\-+!,]+)(?:!!(.*))?$)", group_regex = R"(^!!(?:GROUP)=(.*?)(?:!!(.*))?$)";
    if(startsWith(rule, "!!GROUP="))
    {
        regGetMatch(rule, group_regex, 3, 0, &group, &ret_real_rule);
        real_rule = ret_real_rule;
        return regFind(node.group, group);
    }
    else if(startsWith(rule, "!!GROUPID=") || startsWith(rule, "!!INSERT="))
    {
        int dir = startsWith(rule, "!!INSERT=") ? -1 : 1;
        regGetMatch(rule, groupid_regex, 3, 0, &group, &ret_real_rule);
        real_rule = ret_real_rule;
        return matchRange(group, dir * node.groupID);
    }
    else
        real_rule = rule;
    return true;
}

void nodeRename(nodeInfo &node, const string_array &rename_array)
{
    string_size pos;
    std::string match, rep;
    std::string &remark = node.remarks, original_remark = node.remarks, returned_remark, real_rule;
    duk_context *ctx = NULL;
    defer(duk_destroy_heap(ctx);)

    for(const std::string &x : rename_array)
    {
        if(startsWith(x, "!!script:"))
        {
            if(!ctx)
                ctx = duktape_init();
            std::string script = x.substr(9);
            if(startsWith(script, "path:"))
                script = fileGet(script.substr(5), true);
            if(ctx)
            {
                if(duktape_peval(ctx, script) == 0)
                {
                    duk_get_global_string(ctx, "rename");
                    duktape_push_nodeinfo(ctx, node);
                    if(duk_pcall(ctx, 1) == 0 && !(returned_remark = duktape_get_res_str(ctx)).empty())
                        remark = returned_remark;
                }
                else
                {
                    writeLog(0, "Error when trying to parse rename script:\n" + duktape_get_err_stack(ctx), LOG_LEVEL_ERROR);
                    duk_pop(ctx); // pop err
                }
            }
            continue;
        }
        pos = x.rfind("@");
        match = x.substr(0, pos);
        if(pos != x.npos && pos < x.size())
            rep = x.substr(pos + 1);
        else
            rep.clear();
        if(applyMatcher(match, real_rule, node) && real_rule.size())
            remark = regReplace(remark, real_rule, rep);
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

std::string addEmoji(const nodeInfo &node, const string_array &emoji_array)
{
    std::string real_rule, ret;
    string_size pos;
    duk_context *ctx = NULL;
    defer(duk_destroy_heap(ctx);)

    for(const std::string &x : emoji_array)
    {
        if(startsWith(x, "!!script:"))
        {
            if(!ctx)
                ctx = duktape_init();
            std::string script = x.substr(9);
            if(startsWith(script, "path:"))
                script = fileGet(script.substr(5), true);
            if(ctx)
            {
                if(duktape_peval(ctx, script) == 0)
                {
                    duk_get_global_string(ctx, "getEmoji");
                    duktape_push_nodeinfo(ctx, node);
                    if(duk_pcall(ctx, 1) == 0 && !(ret = duktape_get_res_str(ctx)).empty())
                        return ret + " " + node.remarks;
                }
                else
                {
                    writeLog(0, "Error when trying to parse emoji script:\n" + duktape_get_err_stack(ctx), LOG_LEVEL_ERROR);
                    duk_pop(ctx); // pop err
                }
            }
            continue;
        }
        pos = x.rfind(",");
        if(pos == x.npos)
            continue;
        if(applyMatcher(x.substr(0, pos), real_rule, node) && real_rule.size() && regFind(node.remarks, real_rule))
            return x.substr(pos + 1) + " " + node.remarks;
    }
    return node.remarks;
}

void processRemark(std::string &oldremark, std::string &newremark, string_array &remarks_list, bool proc_comma = true)
{
    if(proc_comma)
    {
        if(oldremark.find(',') != oldremark.npos)
        {
            oldremark.insert(0, "\"");
            oldremark.append("\"");
        }
    }
    newremark = oldremark;
    int cnt = 2;
    while(std::find(remarks_list.begin(), remarks_list.end(), newremark) != remarks_list.end())
    {
        newremark = oldremark + " " + std::to_string(cnt);
        cnt++;
    }
    oldremark = newremark;
}

void rulesetToClash(YAML::Node &base_rule, std::vector<ruleset_content> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name)
{
    string_array allRules;
    std::string rule_group, retrieved_rules, strLine;
    std::stringstream strStrm;
    const std::string field_name = new_field_name ? "rules" : "Rule";
    YAML::Node Rules;
    size_t total_rules = 0;

    if(!overwrite_original_rules && base_rule[field_name].IsDefined())
        Rules = base_rule[field_name];

    for(ruleset_content &x : ruleset_content_array)
    {
        if(gMaxAllowedRules && total_rules > gMaxAllowedRules)
            break;
        rule_group = x.rule_group;
        retrieved_rules = x.rule_content.get();
        if(retrieved_rules.empty())
        {
            writeLog(0, "Failed to fetch ruleset or ruleset is empty: '" + x.rule_path + "'!", LOG_LEVEL_WARNING);
            continue;
        }
        if(startsWith(retrieved_rules, "[]"))
        {
            strLine = retrieved_rules.substr(2);
            if(startsWith(strLine, "FINAL"))
                strLine.replace(0, 5, "MATCH");
            strLine += "," + rule_group;
            if(count_least(strLine, ',', 3))
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            allRules.emplace_back(std::move(strLine));
            total_rules++;
            continue;
        }
        retrieved_rules = convertRuleset(retrieved_rules, x.rule_type);
        char delimiter = getLineBreak(retrieved_rules);

        strStrm.clear();
        strStrm<<retrieved_rules;
        std::string::size_type lineSize;
        while(getline(strStrm, strLine, delimiter))
        {
            if(gMaxAllowedRules && total_rules > gMaxAllowedRules)
                break;
            lineSize = strLine.size();
            if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
                strLine.erase(--lineSize);
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            if(!std::any_of(ClashRuleTypes.begin(), ClashRuleTypes.end(), [strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            strLine += "," + rule_group;
            if(count_least(strLine, ',', 3))
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            allRules.emplace_back(std::move(strLine));
            //Rules.push_back(strLine);
        }
    }

    for(std::string &x : allRules)
    {
        Rules.push_back(x);
    }

    base_rule[field_name] = Rules;
}

std::string rulesetToClashStr(YAML::Node &base_rule, std::vector<ruleset_content> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name)
{
    std::string rule_group, retrieved_rules, strLine;
    std::stringstream strStrm;
    const std::string field_name = new_field_name ? "rules" : "Rule";
    std::string output_content = "\n" + field_name + ":\n";
    size_t total_rules = 0;

    if(!overwrite_original_rules && base_rule[field_name].IsDefined())
    {
        for(size_t i = 0; i < base_rule[field_name].size(); i++)
            output_content += " - " + safe_as<std::string>(base_rule[field_name][i]) + "\n";
    }
    base_rule.remove(field_name);

    for(ruleset_content &x : ruleset_content_array)
    {
        if(gMaxAllowedRules && total_rules > gMaxAllowedRules)
            break;
        rule_group = x.rule_group;
        retrieved_rules = x.rule_content.get();
        if(retrieved_rules.empty())
        {
            writeLog(0, "Failed to fetch ruleset or ruleset is empty: '" + x.rule_path + "'!", LOG_LEVEL_WARNING);
            continue;
        }
        if(startsWith(retrieved_rules, "[]"))
        {
            strLine = retrieved_rules.substr(2);
            if(startsWith(strLine, "FINAL"))
                strLine.replace(0, 5, "MATCH");
            strLine += "," + rule_group;
            if(count_least(strLine, ',', 3))
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            output_content += " - " + strLine + "\n";
            total_rules++;
            continue;
        }
        retrieved_rules = convertRuleset(retrieved_rules, x.rule_type);
        char delimiter = getLineBreak(retrieved_rules);

        strStrm.clear();
        strStrm<<retrieved_rules;
        std::string::size_type lineSize;
        while(getline(strStrm, strLine, delimiter))
        {
            if(gMaxAllowedRules && total_rules > gMaxAllowedRules)
                break;
            lineSize = strLine.size();
            if(lineSize && strLine[lineSize - 1] == '\r')
            {
                strLine.erase(lineSize - 1);
                lineSize = strLine.size();
            }
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            if(std::none_of(ClashRuleTypes.begin(), ClashRuleTypes.end(), [strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            strLine += "," + rule_group;
            if(count_least(strLine, ',', 3))
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            output_content += " - " + strLine + "\n";
            total_rules++;
        }
    }
    return output_content;
}

void rulesetToSurge(INIReader &base_rule, std::vector<ruleset_content> &ruleset_content_array, int surge_ver, bool overwrite_original_rules, std::string remote_path_prefix)
{
    string_array allRules;
    std::string rule_group, rule_path, rule_path_typed, retrieved_rules, strLine;
    std::stringstream strStrm;
    size_t total_rules = 0;

    switch(surge_ver) //other version: -3 for Surfboard, -4 for Loon
    {
    case 0:
        base_rule.SetCurrentSection("RoutingRule"); //Mellow
        break;
    case -1:
        base_rule.SetCurrentSection("filter_local"); //Quantumult X
        break;
    case -2:
        base_rule.SetCurrentSection("TCP"); //Quantumult
        break;
    default:
        base_rule.SetCurrentSection("Rule");
    }

    if(overwrite_original_rules)
    {
        base_rule.EraseSection();
        switch(surge_ver)
        {
        case -1:
            base_rule.EraseSection("filter_remote");
            break;
        case -4:
            base_rule.EraseSection("Remote Rule");
            break;
        }
    }

    const std::string rule_match_regex = "^(.*?,.*?)(,.*)(,.*)$";

    for(ruleset_content &x : ruleset_content_array)
    {
        if(gMaxAllowedRules && total_rules > gMaxAllowedRules)
            break;
        rule_group = x.rule_group;
        rule_path = x.rule_path;
        rule_path_typed = x.rule_path_typed;
        if(rule_path.empty())
        {
            strLine = x.rule_content.get().substr(2);
            if(strLine == "MATCH")
                strLine = "FINAL";
            strLine += "," + rule_group;
            if(surge_ver == -1 || surge_ver == -2)
            {
                if(count_least(strLine, ',', 3) && regReplace(strLine, rule_match_regex, "$2") == ",no-resolve")
                    strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                else
                    strLine = regReplace(strLine, rule_match_regex, "$1$3");
            }
            else
            {
                if(!startsWith(strLine, "AND") && !startsWith(strLine, "OR") && !startsWith(strLine, "NOT") && count_least(strLine, ',', 3))
                    strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
            }
            strLine = replace_all_distinct(strLine, ",,", ",");
            allRules.emplace_back(std::move(strLine));
            total_rules++;
            continue;
        }
        else
        {
            if(surge_ver == -1 && x.rule_type == RULESET_QUANX && isLink(rule_path))
            {
                strLine = rule_path + ", tag=" + rule_group + ", force-policy=" + rule_group + ", enabled=true";
                base_rule.Set("filter_remote", "{NONAME}", strLine);
                continue;
            }
            if(fileExist(rule_path))
            {
                if(surge_ver > 2 && remote_path_prefix.size())
                {
                    strLine = "RULE-SET," + remote_path_prefix + "/getruleset?type=1&url=" + urlsafe_base64_encode(rule_path_typed) + "," + rule_group;
                    if(x.update_interval)
                        strLine += ",update-interval=" + std::to_string(x.update_interval);
                    allRules.emplace_back(std::move(strLine));
                    continue;
                }
                else if(surge_ver == -1 && remote_path_prefix.size())
                {
                    strLine = remote_path_prefix + "/getruleset?type=2&url=" + urlsafe_base64_encode(rule_path_typed) + "&group=" + urlsafe_base64_encode(rule_group);
                    strLine += ", tag=" + rule_group + ", enabled=true";
                    base_rule.Set("filter_remote", "{NONAME}", strLine);
                    continue;
                }
                else if(surge_ver == -4 && remote_path_prefix.size())
                {
                    strLine = remote_path_prefix + "/getruleset?type=1&url=" + urlsafe_base64_encode(rule_path_typed) + "," + rule_group;
                    base_rule.Set("Remote Rule", "{NONAME}", strLine);
                    continue;
                }
            }
            else if(isLink(rule_path))
            {
                if(surge_ver > 2)
                {
                    if(x.rule_type != RULESET_SURGE)
                    {
                        if(remote_path_prefix.size())
                            strLine = "RULE-SET," + remote_path_prefix + "/getruleset?type=1&url=" + urlsafe_base64_encode(rule_path_typed) + "," + rule_group;
                        else
                            continue;
                    }
                    else
                        strLine = "RULE-SET," + rule_path + "," + rule_group;

                    if(x.update_interval)
                        strLine += ",update-interval=" + std::to_string(x.update_interval);

                    allRules.emplace_back(std::move(strLine));
                    continue;
                }
                else if(surge_ver == -1 && remote_path_prefix.size())
                {
                    strLine = remote_path_prefix + "/getruleset?type=2&url=" + urlsafe_base64_encode(rule_path_typed) + "&group=" + urlsafe_base64_encode(rule_group);
                    strLine += ", tag=" + rule_group + ", enabled=true";
                    base_rule.Set("filter_remote", "{NONAME}", strLine);
                    continue;
                }
                else if(surge_ver == -4)
                {
                    strLine = rule_path + "," + rule_group;
                    base_rule.Set("Remote Rule", "{NONAME}", strLine);
                    continue;
                }
            }
            else
                continue;
            retrieved_rules = x.rule_content.get();
            if(retrieved_rules.empty())
            {
                writeLog(0, "Failed to fetch ruleset or ruleset is empty: '" + x.rule_path + "'!", LOG_LEVEL_WARNING);
                continue;
            }

            retrieved_rules = convertRuleset(retrieved_rules, x.rule_type);
            char delimiter = getLineBreak(retrieved_rules);

            strStrm.clear();
            strStrm<<retrieved_rules;
            std::string::size_type lineSize;
            while(getline(strStrm, strLine, delimiter))
            {
                if(gMaxAllowedRules && total_rules > gMaxAllowedRules)
                    break;
                lineSize = strLine.size();
                if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
                    strLine.erase(--lineSize);
                if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                    continue;

                /// remove unsupported types
                switch(surge_ver)
                {
                case -2:
                    if(startsWith(strLine, "IP-CIDR6"))
                        continue;
                    [[fallthrough]];
                case -1:
                    if(!std::any_of(QuanXRuleTypes.begin(), QuanXRuleTypes.end(), [strLine](std::string type){return startsWith(strLine, type);}))
                        continue;
                    break;
                case -3:
                    if(!std::any_of(SurfRuleTypes.begin(), SurfRuleTypes.end(), [strLine](std::string type){return startsWith(strLine, type);}))
                        continue;
                    break;
                default:
                    if(surge_ver > 2)
                    {
                        if(!std::any_of(SurgeRuleTypes.begin(), SurgeRuleTypes.end(), [strLine](std::string type){return startsWith(strLine, type);}))
                            continue;
                    }
                    else
                    {
                        if(!std::any_of(Surge2RuleTypes.begin(), Surge2RuleTypes.end(), [strLine](std::string type){return startsWith(strLine, type);}))
                            continue;
                    }
                }

                strLine += "," + rule_group;
                if(surge_ver == -1 || surge_ver == -2)
                {
                    if(startsWith(strLine, "IP-CIDR6"))
                        strLine.replace(0, 8, "IP6-CIDR");
                    if(count_least(strLine, ',', 3) && regReplace(strLine, rule_match_regex, "$2") == ",no-resolve")
                        strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                    else
                        strLine = regReplace(strLine, rule_match_regex, "$1$3");
                }
                else
                {
                    if(!startsWith(strLine, "AND") && !startsWith(strLine, "OR") && !startsWith(strLine, "NOT") && count_least(strLine, ',', 3))
                        strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                }
                allRules.emplace_back(std::move(strLine));
                total_rules++;
            }
        }
    }

    for(std::string &x : allRules)
    {
        base_rule.Set("{NONAME}", x);
    }
}

void parseGroupTimes(const std::string &src, int *interval, int *tolerance, int *timeout)
{
    std::vector<int*> ptrs;
    ptrs.push_back(interval);
    ptrs.push_back(timeout);
    ptrs.push_back(tolerance);
    string_size bpos = 0, epos = src.find(",");
    for(int *x : ptrs)
    {
        if(x != NULL)
            *x = to_int(src.substr(bpos, epos - bpos), 0);
        if(epos != src.npos)
        {
            bpos = epos + 1;
            epos = src.find(",", bpos);
        }
        else
            return;
    }
    return;
}

void groupGenerate(std::string &rule, std::vector<nodeInfo> &nodelist, string_array &filtered_nodelist, bool add_direct)
{
    std::string real_rule;
    if(startsWith(rule, "[]") && add_direct)
    {
        filtered_nodelist.emplace_back(rule.substr(2));
    }
    else if(startsWith(rule, "script:"))
    {
        duk_context *ctx = duktape_init();
        if(ctx)
        {
            defer(duk_destroy_heap(ctx);)
            std::string script = fileGet(rule.substr(7), true);
            if(duktape_peval(ctx, script) == 0)
            {
                duk_get_global_string(ctx, "filter");
                duk_idx_t arr_idx = duk_push_array(ctx), node_idx = 0;
                for(nodeInfo &x : nodelist)
                {
                    duktape_push_nodeinfo_arr(ctx, x, -1);
                    duk_put_prop_index(ctx, arr_idx, node_idx++);
                }
                if(duk_pcall(ctx, 1) == 0)
                {
                    std::string result_list = duktape_get_res_str(ctx);
                    filtered_nodelist = split(regTrim(result_list), "\n");
                }
                else
                {
                    writeLog(0, "Error when trying to evaluate script:\n" + duktape_get_err_stack(ctx), LOG_LEVEL_ERROR);
                    duk_pop(ctx);
                }
            }
            else
            {
                writeLog(0, "Error when trying to parse script:\n" + duktape_get_err_stack(ctx), LOG_LEVEL_ERROR);
                duk_pop(ctx);
            }
        }
    }
    else
    {
        for(nodeInfo &x : nodelist)
        {
            if(applyMatcher(rule, real_rule, x) && (real_rule.empty() || regFind(x.remarks, real_rule)) && std::find(filtered_nodelist.begin(), filtered_nodelist.end(), x.remarks) == filtered_nodelist.end())
                filtered_nodelist.emplace_back(x.remarks);
        }
    }
}

void preprocessNodes(std::vector<nodeInfo> &nodes, const extra_settings &ext)
{
    std::for_each(nodes.begin(), nodes.end(), [ext](nodeInfo &x)
    {
        if(ext.remove_emoji)
            x.remarks = trim(removeEmoji(x.remarks));

        nodeRename(x, ext.rename_array);

        if(ext.add_emoji)
            x.remarks = addEmoji(x, ext.emoji_array);
    });

    if(ext.sort_flag)
    {
        bool failed = true;
        if(ext.sort_script.size())
        {
            try
            {
                duk_context *ctx = duktape_init();
                if(ctx)
                {
                    defer(duk_destroy_heap(ctx);)
                    if(duktape_peval(ctx, ext.sort_script) == 0)
                    {
                        auto comparer = [&](const nodeInfo &a, const nodeInfo &b)
                        {
                            if(a.linkType < 1 || a.linkType > 5)
                                return 1;
                            if(b.linkType < 1 || b.linkType > 5)
                                return 0;
                            duk_get_global_string(ctx, "compare");
                            /// push 2 nodeinfo
                            duktape_push_nodeinfo(ctx, a);
                            duktape_push_nodeinfo(ctx, b);
                            /// call function
                            duk_pcall(ctx, 2);
                            return duktape_get_res_int(ctx);
                        };
                        std::sort(nodes.begin(), nodes.end(), comparer);
                        failed = false;
                    }
                    else
                    {
                        writeLog(0, "Error when trying to parse script:\n" + duktape_get_err_stack(ctx), LOG_LEVEL_ERROR);
                        duk_pop(ctx); /// pop err
                    }
                }
            }
            catch (std::exception&)
            {
                //failed
            }
        }
        if(failed) std::sort(nodes.begin(), nodes.end(), [](const nodeInfo &a, const nodeInfo &b)
        {
            return a.remarks < b.remarks;
        });
    }
}

void netchToClash(std::vector<nodeInfo> &nodes, YAML::Node &yamlnode, const string_array &extra_proxy_group, bool clashR, const extra_settings &ext)
{
    YAML::Node proxies, singleproxy, singlegroup, original_groups;
    rapidjson::Document json;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, edge, path, quicsecure, quicsecret;
    tribool udp, scv;
    std::vector<nodeInfo> nodelist;
    bool tlssecure;
    string_array vArray, remarks_list, filtered_nodelist;
    /// proxies style
    bool block = false, compact = false;
    switch(hash_(ext.clash_proxies_style))
    {
    case "block"_hash:
        block = true;
        break;
    default:
    case "flow"_hash:
        break;
    case "compact"_hash:
        compact = true;
        break;
    }

    for(nodeInfo &x : nodes)
    {
        singleproxy.reset();
        json.Parse(x.proxyStr.data());

        type = GetMember(json, "Type");
        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;

        processRemark(x.remarks, remark, remarks_list, false);

        hostname = GetMember(json, "Hostname");
        port = GetMember(json, "Port");
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");

        udp = ext.udp;
        scv = ext.skip_cert_verify;
        udp.define(GetMember(json, "EnableUDP"));
        scv.define(GetMember(json, "AllowInsecure"));

        singleproxy["name"] = remark;
        singleproxy["server"] = hostname;
        singleproxy["port"] = (unsigned short)to_int(port);

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDSS:
            //latest clash core removed support for chacha20 encryption
            if(ext.filter_deprecated && method == "chacha20")
                continue;
            plugin = GetMember(json, "Plugin");
            pluginopts = replace_all_distinct(GetMember(json, "PluginOption"), ";", "&");
            singleproxy["type"] = "ss";
            singleproxy["cipher"] = method;
            singleproxy["password"] = password;
            if(std::all_of(password.begin(), password.end(), ::isdigit) && !password.empty())
                singleproxy["password"].SetTag("str");
            switch(hash_(plugin))
            {
            case "simple-obfs"_hash:
            case "obfs-local"_hash:
                singleproxy["plugin"] = "obfs";
                singleproxy["plugin-opts"]["mode"] = UrlDecode(getUrlArg(pluginopts, "obfs"));
                singleproxy["plugin-opts"]["host"] = UrlDecode(getUrlArg(pluginopts, "obfs-host"));
                break;
            case "v2ray-plugin"_hash:
                singleproxy["plugin"] = "v2ray-plugin";
                singleproxy["plugin-opts"]["mode"] = getUrlArg(pluginopts, "mode");
                singleproxy["plugin-opts"]["host"] = getUrlArg(pluginopts, "host");
                singleproxy["plugin-opts"]["path"] = getUrlArg(pluginopts, "path");
                singleproxy["plugin-opts"]["tls"] = pluginopts.find("tls") != pluginopts.npos;
                singleproxy["plugin-opts"]["mux"] = pluginopts.find("mux") != pluginopts.npos;
                if(!scv.is_undef())
                    singleproxy["plugin-opts"]["skip-cert-verify"] = scv.get();
                break;
            }
            break;
        case SPEEDTEST_MESSAGE_FOUNDVMESS:
            id = GetMember(json, "UserID");
            aid = GetMember(json, "AlterID");
            transproto = GetMember(json, "TransferProtocol");
            host = GetMember(json, "Host");
            edge = GetMember(json, "Edge");
            path = GetMember(json, "Path");
            tlssecure = GetMember(json, "TLSSecure") == "true";
            singleproxy["type"] = "vmess";
            singleproxy["uuid"] = id;
            singleproxy["alterId"] = stoi(aid);
            singleproxy["cipher"] = method;
            singleproxy["tls"] = tlssecure;
            if(!scv.is_undef())
                singleproxy["skip-cert-verify"] = scv.get();
            switch(hash_(transproto))
            {
            case "tcp"_hash:
                break;
            case "ws"_hash:
                singleproxy["network"] = transproto;
                singleproxy["ws-path"] = path;
                if(host.size())
                    singleproxy["ws-headers"]["Host"] = host;
                if(edge.size())
                    singleproxy["ws-headers"]["Edge"] = edge;
                break;
            case "http"_hash:
                singleproxy["network"] = transproto;
                singleproxy["http-opts"]["method"] = "GET";
                singleproxy["http-opts"]["path"].push_back(path);
                if(host.size())
                    singleproxy["http-opts"]["headers"]["Host"].push_back(host);
                if(edge.size())
                    singleproxy["http-opts"]["headers"]["Edge"].push_back(edge);
                break;
            default:
                continue;
            }
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            //ignoring all nodes with unsupported obfs, protocols and encryption
            protocol = GetMember(json, "Protocol");
            obfs = GetMember(json, "OBFS");
            if(ext.filter_deprecated)
            {
                if(!clashR && std::find(clash_ssr_ciphers.cbegin(), clash_ssr_ciphers.cend(), method) == clash_ssr_ciphers.cend())
                    continue;
                if(std::find(clashr_protocols.cbegin(), clashr_protocols.cend(), protocol) == clashr_protocols.cend())
                    continue;
                if(std::find(clashr_obfs.cbegin(), clashr_obfs.cend(), obfs) == clashr_obfs.cend())
                    continue;
            }

            protoparam = GetMember(json, "ProtocolParam");
            obfsparam = GetMember(json, "OBFSParam");
            singleproxy["type"] = "ssr";
            singleproxy["cipher"] = method;
            singleproxy["password"] = password;
            if(std::all_of(password.begin(), password.end(), ::isdigit) && !password.empty())
                singleproxy["password"].SetTag("str");
            singleproxy["protocol"] = protocol;
            singleproxy["obfs"] = obfs;
            if(clashR)
            {
                singleproxy["protocolparam"] = protoparam;
                singleproxy["obfsparam"] = obfsparam;
            }
            else
            {
                singleproxy["protocol-param"] = protoparam;
                singleproxy["obfs-param"] = obfsparam;
            }
            break;
        case SPEEDTEST_MESSAGE_FOUNDSOCKS:
            singleproxy["type"] = "socks5";
            if(!username.empty())
                singleproxy["username"] = username;
            if(!password.empty())
            {
                singleproxy["password"] = password;
                if(std::all_of(password.begin(), password.end(), ::isdigit))
                    singleproxy["password"].SetTag("str");
            }
            if(!scv.is_undef())
                singleproxy["skip-cert-verify"] = scv.get();
            break;
        case SPEEDTEST_MESSAGE_FOUNDHTTP:
            singleproxy["type"] = "http";
            if(!username.empty())
                singleproxy["username"] = username;
            if(!password.empty())
            {
                singleproxy["password"] = password;
                if(std::all_of(password.begin(), password.end(), ::isdigit))
                    singleproxy["password"].SetTag("str");
            }
            singleproxy["tls"] = type == "HTTPS";
            if(!scv.is_undef())
                singleproxy["skip-cert-verify"] = scv.get();
            break;
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            host = GetMember(json, "Host");
            singleproxy["type"] = "trojan";
            singleproxy["password"] = password;
            if(host.size())
                singleproxy["sni"] = host;
            if(std::all_of(password.begin(), password.end(), ::isdigit) && !password.empty())
                singleproxy["password"].SetTag("str");
            if(!scv.is_undef())
                singleproxy["skip-cert-verify"] = scv.get();
            break;
        case SPEEDTEST_MESSAGE_FOUNDSNELL:
            obfs = GetMember(json, "OBFS");
            host = GetMember(json, "Host");
            singleproxy["type"] = "snell";
            singleproxy["psk"] = password;
            if(obfs.size())
            {
                singleproxy["obfs-opts"]["mode"] = obfs;
                if(host.size())
                    singleproxy["obfs-opts"]["host"] = host;
            }
            if(std::all_of(password.begin(), password.end(), ::isdigit) && !password.empty())
                singleproxy["password"].SetTag("str");
            break;
        default:
            continue;
        }

        if(udp)
            singleproxy["udp"] = true;
        if(block)
            singleproxy.SetStyle(YAML::EmitterStyle::Block);
        else
            singleproxy.SetStyle(YAML::EmitterStyle::Flow);
        proxies.push_back(singleproxy);
        remarks_list.emplace_back(std::move(remark));
        nodelist.emplace_back(x);
    }

    if(compact)
        proxies.SetStyle(YAML::EmitterStyle::Flow);

    if(ext.nodelist)
    {
        YAML::Node provider;
        provider["proxies"] = proxies;
        yamlnode.reset(provider);
        return;
    }

    if(ext.clash_new_field_name)
        yamlnode["proxies"] = proxies;
    else
        yamlnode["Proxy"] = proxies;

    string_array providers;

    for(const std::string &x : extra_proxy_group)
    {
        singlegroup.reset();
        eraseElements(filtered_nodelist);
        eraseElements(providers);
        unsigned int rules_upper_bound = 0;

        vArray = split(x, "`");
        if(vArray.size() < 3)
            continue;

        singlegroup["name"] = vArray[0];
        singlegroup["type"] = vArray[1];

        int interval = 0, tolerance = 0;
        rules_upper_bound = vArray.size();
        switch(hash_(vArray[1]))
        {
        case "select"_hash:
        case "relay"_hash:
            break;
        case "url-test"_hash:
        case "fallback"_hash:
        case "load-balance"_hash:
            if(rules_upper_bound < 5)
                continue;
            rules_upper_bound -= 2;
            singlegroup["url"] = vArray[rules_upper_bound];
            parseGroupTimes(vArray[rules_upper_bound + 1], &interval, &tolerance, NULL);
            if(interval)
                singlegroup["interval"] = interval;
            if(tolerance)
                singlegroup["tolerance"] = tolerance;
            break;
        default:
            continue;
        }

        for(unsigned int i = 2; i < rules_upper_bound; i++)
        {
            if(startsWith(vArray[i], "!!PROVIDER="))
            {
                string_array list = split(vArray[i].substr(11), ",");
                providers.reserve(providers.size() + list.size());
                std::move(list.begin(), list.end(), std::back_inserter(providers));
            }
            else
                groupGenerate(vArray[i], nodelist, filtered_nodelist, true);
        }

        if(providers.size())
            singlegroup["use"] = providers;
        else
        {
            if(filtered_nodelist.empty())
                filtered_nodelist.emplace_back("DIRECT");
        }
        if(!filtered_nodelist.empty())
            singlegroup["proxies"] = filtered_nodelist;
        //singlegroup.SetStyle(YAML::EmitterStyle::Flow);

        bool replace_flag = false;
        for(unsigned int i = 0; i < original_groups.size(); i++)
        {
            if(original_groups[i]["name"].as<std::string>() == vArray[0])
            {
                original_groups[i] = singlegroup;
                replace_flag = true;
                break;
            }
        }
        if(!replace_flag)
            original_groups.push_back(singlegroup);
    }

    if(ext.clash_new_field_name)
        yamlnode["proxy-groups"] = original_groups;
    else
        yamlnode["Proxy Group"] = original_groups;
}

std::string netchToClash(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, bool clashR, const extra_settings &ext)
{
    YAML::Node yamlnode;

    try
    {
        yamlnode = YAML::Load(base_conf);
    }
    catch (std::exception &e)
    {
        writeLog(0, std::string("Clash base loader failed with error: ") + e.what(), LOG_LEVEL_ERROR);
        return std::string();
    }

    netchToClash(nodes, yamlnode, extra_proxy_group, clashR, ext);

    if(ext.nodelist)
        return YAML::Dump(yamlnode);

    /*
    if(ext.enable_rule_generator)
        rulesetToClash(yamlnode, ruleset_content_array, ext.overwrite_original_rules, ext.clash_new_field_name);

    return YAML::Dump(yamlnode);
    */
    if(!ext.enable_rule_generator)
        return YAML::Dump(yamlnode);

    if(ext.managed_config_prefix.size() || ext.clash_script)
    {
        if(yamlnode["mode"].IsDefined())
        {
            if(ext.clash_new_field_name)
                yamlnode["mode"] = ext.clash_script ? "script" : "rule";
            else
                yamlnode["mode"] = ext.clash_script ? "Script" : "Rule";
        }

        renderClashScript(yamlnode, ruleset_content_array, ext.managed_config_prefix, ext.clash_script, ext.overwrite_original_rules, ext.clash_classical_ruleset);
        return YAML::Dump(yamlnode);
    }

    std::string output_content = rulesetToClashStr(yamlnode, ruleset_content_array, ext.overwrite_original_rules, ext.clash_new_field_name);
    output_content.insert(0, YAML::Dump(yamlnode));

    return output_content;
}

std::string netchToSurge(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, int surge_ver, const extra_settings &ext)
{
    rapidjson::Document json;
    INIReader ini;
    std::string proxy;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, edge, path, quicsecure, quicsecret;
    std::string output_nodelist;
    tribool udp, tfo, scv, tls13;
    std::vector<nodeInfo> nodelist;
    unsigned short local_port = 1080;
    bool tlssecure;

    string_array vArray, remarks_list, filtered_nodelist, args;

    ini.store_any_line = true;
    // filter out sections that requires direct-save
    ini.AddDirectSaveSection("General");
    ini.AddDirectSaveSection("Replica");
    ini.AddDirectSaveSection("Rule");
    ini.AddDirectSaveSection("MITM");
    ini.AddDirectSaveSection("Script");
    ini.AddDirectSaveSection("Host");
    ini.AddDirectSaveSection("URL Rewrite");
    ini.AddDirectSaveSection("Header Rewrite");
    if(ini.Parse(base_conf) != 0 && !ext.nodelist)
    {
        writeLog(0, "Surge base loader failed with error: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return std::string();
    }

    ini.SetCurrentSection("Proxy");
    ini.EraseSection();
    ini.Set("{NONAME}", "DIRECT = direct");

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");

        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;

        processRemark(x.remarks, remark, remarks_list);

        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)to_int(GetMember(json, "Port")));
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");

        udp = ext.udp;
        tfo = ext.tfo;
        scv = ext.skip_cert_verify;
        tls13 = ext.tls13;
        udp.define(GetMember(json, "EnableUDP"));
        tfo.define(GetMember(json, "EnableTFO"));
        scv.define(GetMember(json, "AllowInsecure"));
        tls13.define(GetMember(json, "TLS13"));

        proxy.clear();

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDSS:
            plugin = GetMember(json, "Plugin");
            pluginopts = GetMember(json, "PluginOption");
            if(surge_ver >= 3)
            {
                proxy = "ss, " + hostname + ", " + port + ", encrypt-method=" + method + ", password=" + password;
            }
            else
            {
                proxy = "custom, "  + hostname + ", " + port + ", " + method + ", " + password + ", https://github.com/ConnersHua/SSEncrypt/raw/master/SSEncrypt.module";
            }
            if(plugin.size())
            {
                switch(hash_(plugin))
                {
                case "simple-obfs"_hash:
                case "obfs-local"_hash:
                    if(pluginopts.size())
                        proxy += "," + replace_all_distinct(pluginopts, ";", ",");
                    break;
                default:
                    continue;
                }
            }
            break;
        case SPEEDTEST_MESSAGE_FOUNDVMESS:
            if(surge_ver < 4 && surge_ver != -3)
                continue;
            id = GetMember(json, "UserID");
            aid = GetMember(json, "AlterID");
            transproto = GetMember(json, "TransferProtocol");
            host = GetMember(json, "Host");
            edge = GetMember(json, "Edge");
            path = GetMember(json, "Path");
            tlssecure = GetMember(json, "TLSSecure") == "true";
            proxy = "vmess, " + hostname + ", " + port + ", username=" + id + ", tls=" + (tlssecure ? "true" : "false");
            if(tlssecure && !tls13.is_undef())
                proxy += ", tls13=" + std::string(tls13 ? "true" : "false");
            switch(hash_(transproto))
            {
            case "tcp"_hash:
                break;
            case "ws"_hash:
                proxy += ", ws=true, ws-path=" + path + ", sni=" + host + ", ws-headers=Host:" + host;
                if(edge.size())
                    proxy += "|Edge:" + edge;
                break;
            default:
                continue;
            }
            if(!scv.is_undef())
                proxy += ", skip-cert-verify=" + std::string(scv.get() ? "1" : "0");
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            if(ext.surge_ssr_path.empty() || surge_ver < 2)
                continue;
            protocol = GetMember(json, "Protocol");
            protoparam = GetMember(json, "ProtocolParam");
            obfs = GetMember(json, "OBFS");
            obfsparam = GetMember(json, "OBFSParam");
            proxy = "external, exec=\"" + ext.surge_ssr_path + "\", args=\"";
            args = {"-l", std::to_string(local_port), "-s", hostname, "-p", port, "-m", method, "-k", password, "-o", obfs, "-O", protocol};
            if(obfsparam.size())
            {
                args.emplace_back("-g");
                args.emplace_back(std::move(obfsparam));
            }
            if(protoparam.size())
            {
                args.emplace_back("-G");
                args.emplace_back(std::move(protoparam));
            }
            proxy += std::accumulate(std::next(args.begin()), args.end(), args[0], [](std::string a, std::string b)
            {
                return std::move(a) + "\", args=\"" + std::move(b);
            });
            proxy += "\", local-port=" + std::to_string(local_port);
            if(isIPv4(hostname) || isIPv6(hostname))
                proxy += ", addresses=" + hostname;
            else if(gSurgeResolveHostname)
                proxy += ", addresses=" + hostnameToIPAddr(hostname);
            local_port++;
            break;
        case SPEEDTEST_MESSAGE_FOUNDSOCKS:
            proxy = "socks5, " + hostname + ", " + port;
            if(username.size())
                proxy += ", username=" + username;
            if(password.size())
                proxy += ", password=" + password;
            if(!scv.is_undef())
                proxy += ", skip-cert-verify=" + std::string(scv.get() ? "1" : "0");
            break;
        case SPEEDTEST_MESSAGE_FOUNDHTTP:
            proxy = "http, " + hostname + ", " + port;
            if(username.size())
                proxy += ", username=" + username;
            if(password.size())
                proxy += ", password=" + password;
            proxy += std::string(", tls=") + (type == "HTTPS" ? "true" : "false");
            if(!scv.is_undef())
                proxy += ", skip-cert-verify=" + std::string(scv.get() ? "1" : "0");
            break;
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            if(surge_ver < 4)
                continue;
            host = GetMember(json, "Host");
            proxy = "trojan, " + hostname + ", " + port + ", password=" + password;
            if(host.size())
                proxy += ", sni=" + host;
            if(!scv.is_undef())
                proxy += ", skip-cert-verify=" + std::string(scv.get() ? "1" : "0");
            break;
        case SPEEDTEST_MESSAGE_FOUNDSNELL:
            obfs = GetMember(json, "OBFS");
            host = GetMember(json, "Host");
            proxy = "snell, " + hostname + ", " + port + ", psk=" + password;
            if(obfs.size())
                proxy += ", obfs=" + obfs + ", obfs-host=" + host;
            break;
        default:
            continue;
        }

        if(!tfo.is_undef())
            proxy += ", tfo=" + tfo.get_str();
        if(!udp.is_undef())
            proxy += ", udp-relay=" + udp.get_str();

        if(ext.nodelist)
            output_nodelist += remark + " = " + proxy + "\n";
        else
        {
            ini.Set("{NONAME}", remark + " = " + proxy);
            nodelist.emplace_back(x);
        }
        remarks_list.emplace_back(std::move(remark));
    }

    if(ext.nodelist)
        return output_nodelist;

    ini.SetCurrentSection("Proxy Group");
    ini.EraseSection();
    for(const std::string &x : extra_proxy_group)
    {
        //group pref
        std::string url;
        int interval = 0, tolerance = 0, timeout = 0;
        eraseElements(filtered_nodelist);
        unsigned int rules_upper_bound = 0;
        url.clear();
        proxy.clear();

        vArray = split(x, "`");
        if(vArray.size() < 3)
            continue;

        rules_upper_bound = vArray.size();
        switch(hash_(vArray[1]))
        {
        case "select"_hash:
            break;
        case "load-balance"_hash:
            if(surge_ver < 1)
                continue;
            [[fallthrough]];
        case "url-test"_hash:
        case "fallback"_hash:
            if(rules_upper_bound < 5)
                continue;
            rules_upper_bound -= 2;
            url = vArray[rules_upper_bound];
            parseGroupTimes(vArray[rules_upper_bound + 1], &interval, &tolerance, &timeout);
            break;
        case "ssid"_hash:
            if(rules_upper_bound < 4)
                continue;
            proxy = vArray[1] + ",default=" + vArray[2] + ",";
            proxy += std::accumulate(vArray.begin() + 4, vArray.end(), vArray[3], [](std::string a, std::string b)
            {
                return std::move(a) + "," + std::move(b);
            });
            ini.Set("{NONAME}", vArray[0] + " = " + proxy); //insert order
            continue;
        default:
            continue;
        }

        for(unsigned int i = 2; i < rules_upper_bound; i++)
            groupGenerate(vArray[i], nodelist, filtered_nodelist, true);

        if(!filtered_nodelist.size())
            filtered_nodelist.emplace_back("DIRECT");

        if(filtered_nodelist.size() == 1)
        {
            proxy = toLower(filtered_nodelist[0]);
            switch(hash_(proxy))
            {
            case "direct"_hash:
            case "reject"_hash:
            case "reject-tinygif"_hash:
                ini.Set("Proxy", "{NONAME}", vArray[0] + " = " + proxy);
                continue;
            }
        }

        proxy = vArray[1] + ",";
        proxy += std::accumulate(std::next(filtered_nodelist.cbegin()), filtered_nodelist.cend(), filtered_nodelist[0], [](std::string a, std::string b)
        {
            return std::move(a) + "," + std::move(b);
        });
        if(vArray[1] == "url-test" || vArray[1] == "fallback")
        {
            proxy += ",url=" + url + ",interval=" + std::to_string(interval);
            if(tolerance > 0)
                proxy += ",tolerance=" + std::to_string(tolerance);
            if(timeout > 0)
                proxy += ",timeout=" + std::to_string(timeout);
        }
        else if(vArray[1] == "load-balance")
            proxy += ",url=" + url;

        ini.Set("{NONAME}", vArray[0] + " = " + proxy); //insert order
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, surge_ver, ext.overwrite_original_rules, ext.managed_config_prefix);

    return ini.ToString();
}

std::string netchToSingle(std::vector<nodeInfo> &nodes, int types, const extra_settings &ext)
{
    /// types: SS=1 SSR=2 VMess=4 Trojan=8
    rapidjson::Document json;
    std::string remark, hostname, port, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, path, quicsecure, quicsecret;
    std::string proxyStr, allLinks;
    bool tlssecure, ss = GETBIT(types, 1), ssr = GETBIT(types, 2), vmess = GETBIT(types, 3), trojan = GETBIT(types, 4);

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());

        remark = x.remarks;
        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)to_int(GetMember(json, "Port")));
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");
        plugin = GetMember(json, "Plugin");
        pluginopts = GetMember(json, "PluginOption");
        protocol = GetMember(json, "Protocol");
        protoparam = GetMember(json, "ProtocolParam");
        obfs = GetMember(json, "OBFS");
        obfsparam = GetMember(json, "OBFSParam");

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDSS:
            if(ss)
            {
                proxyStr = "ss://" + urlsafe_base64_encode(method + ":" + password) + "@" + hostname + ":" + port;
                if(plugin.size() && pluginopts.size())
                {
                    proxyStr += "/?plugin=" + UrlEncode(plugin + ";" + pluginopts);
                }
                proxyStr += "#" + UrlEncode(remark);
            }
            else if(ssr)
            {
                if(std::count(ssr_ciphers.begin(), ssr_ciphers.end(), method) > 0 && !GetMember(json, "Plugin").size() && !GetMember(json, "Plugin").size())
                    proxyStr = "ssr://" + urlsafe_base64_encode(hostname + ":" + port + ":origin:" + method + ":plain:" + urlsafe_base64_encode(password) \
                               + "/?group=" + urlsafe_base64_encode(x.group) + "&remarks=" + urlsafe_base64_encode(remark));
            }
            else
                continue;
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            if(ssr)
            {
                proxyStr = "ssr://" + urlsafe_base64_encode(hostname + ":" + port + ":" + protocol + ":" + method + ":" + obfs + ":" + urlsafe_base64_encode(password) \
                           + "/?group=" + urlsafe_base64_encode(x.group) + "&remarks=" + urlsafe_base64_encode(remark) \
                           + "&obfsparam=" + urlsafe_base64_encode(obfsparam) + "&protoparam=" + urlsafe_base64_encode(protoparam));
            }
            else if(ss)
            {
                if(std::count(ss_ciphers.begin(), ss_ciphers.end(), method) > 0 && protocol == "origin" && obfs == "plain")
                    proxyStr = "ss://" + urlsafe_base64_encode(method + ":" + password) + "@" + hostname + ":" + port + "#" + UrlEncode(remark);
            }
            else
                continue;
            break;
        case SPEEDTEST_MESSAGE_FOUNDVMESS:
            if(!vmess)
                continue;
            id = GetMember(json, "UserID");
            aid = GetMember(json, "AlterID");
            transproto = GetMember(json, "TransferProtocol");
            host = GetMember(json, "Host");
            path = GetMember(json, "Path");
            faketype = GetMember(json, "FakeType");
            tlssecure = GetMember(json, "TLSSecure") == "true";
            proxyStr = "vmess://" + base64_encode(vmessLinkConstruct(remark, hostname, port, faketype, id, aid, transproto, path, host, tlssecure ? "tls" : ""));
            break;
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            if(!trojan)
                continue;
            proxyStr = "trojan://" + password + "@" + hostname + ":" + port + "#" + UrlEncode(remark);
            break;
        default:
            continue;
        }
        allLinks += proxyStr + "\n";
    }

    if(ext.nodelist)
        return allLinks;
    else
        return base64_encode(allLinks);
}

std::string netchToSSSub(std::string &base_conf, std::vector<nodeInfo> &nodes, const extra_settings &ext)
{
    rapidjson::Document json, base;
    std::string remark, hostname, password, method;
    std::string plugin, pluginopts;
    std::string protocol, obfs;
    std::string output_content;

    rapidjson::Document::AllocatorType &alloc = json.GetAllocator();
    json.SetObject();
    json.AddMember("remarks", "", alloc);
    json.AddMember("server", "", alloc);
    json.AddMember("server_port", 0, alloc);
    json.AddMember("method", "", alloc);
    json.AddMember("password", "", alloc);
    json.AddMember("plugin", "", alloc);
    json.AddMember("plugin_opts", "", alloc);

    rapidjson::ParseResult result = base.Parse(base_conf.data());
    if(result)
    {
        for(auto iter = base.MemberBegin(); iter != base.MemberEnd(); iter++)
            json.AddMember(iter->name, iter->value, alloc);
    }
    else
        writeLog(0, std::string("SIP008 base loader failed with error: ") + rapidjson::GetParseError_En(result.Code()) + " (" + std::to_string(result.Offset()) + ")", LOG_LEVEL_ERROR);

    rapidjson::Value jsondata;
    jsondata = json.Move();

    output_content = "[";
    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());

        remark = x.remarks;
        hostname = x.server;
        int port = (unsigned short)to_int(GetMember(json, "Port"));
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");
        plugin = GetMember(json, "Plugin");
        pluginopts = GetMember(json, "PluginOption");
        protocol = GetMember(json, "Protocol");
        obfs = GetMember(json, "OBFS");

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDSS:
            if(plugin == "simple-obfs")
                plugin = "obfs-local";
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            if(std::count(ss_ciphers.begin(), ss_ciphers.end(), method) > 0 && protocol == "origin" && obfs == "plain")
                continue;
            break;
        default:
            continue;
        }
        jsondata["remarks"].SetString(rapidjson::StringRef(remark.c_str(), remark.size()));
        jsondata["server"].SetString(rapidjson::StringRef(hostname.c_str(), hostname.size()));
        jsondata["server_port"] = port;
        jsondata["password"].SetString(rapidjson::StringRef(password.c_str(), password.size()));
        jsondata["method"].SetString(rapidjson::StringRef(method.c_str(), method.size()));
        jsondata["plugin"].SetString(rapidjson::StringRef(plugin.c_str(), plugin.size()));
        jsondata["plugin_opts"].SetString(rapidjson::StringRef(pluginopts.c_str(), pluginopts.size()));
        output_content += SerializeObject(jsondata) + ",";
    }
    if(output_content.size() > 1)
        output_content.erase(output_content.size() - 1);
    output_content += "]";
    return output_content;
}

std::string netchToQuan(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext)
{
    INIReader ini;
    ini.store_any_line = true;
    if(!ext.nodelist && ini.Parse(base_conf) != 0)
    {
        writeLog(0, "Quantumult base loader failed with error: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return std::string();
    }

    netchToQuan(nodes, ini, ruleset_content_array, extra_proxy_group, ext);

    if(ext.nodelist)
    {
        string_array allnodes;
        std::string allLinks;
        ini.GetAll("SERVER", "{NONAME}", allnodes);
        if(allnodes.size())
            allLinks = std::accumulate(std::next(allnodes.begin()), allnodes.end(), allnodes[0], [](std::string a, std::string b)
            {
                return std::move(a) + "\n" + std::move(b);
            });
        return base64_encode(allLinks);
    }
    return ini.ToString();
}

void netchToQuan(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext)
{
    rapidjson::Document json;
    std::string type;
    std::string remark, hostname, port, method, username, password;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, edge, path, quicsecure, quicsecret;
    std::string proxyStr;
    bool tlssecure;
    tribool scv;
    std::vector<nodeInfo> nodelist;
    string_array remarks_list;

    ini.SetCurrentSection("SERVER");
    ini.EraseSection();
    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");

        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;

        processRemark(x.remarks, remark, remarks_list);

        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)to_int(GetMember(json, "Port")));
        method = GetMember(json, "EncryptMethod");
        password = GetMember(json, "Password");

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDVMESS:
            id = GetMember(json, "UserID");
            aid = GetMember(json, "AlterID");
            transproto = GetMember(json, "TransferProtocol");
            host = GetMember(json, "Host");
            path = GetMember(json, "Path");
            edge = GetMember(json, "Edge");
            faketype = GetMember(json, "FakeType");
            tlssecure = GetMember(json, "TLSSecure") == "true";

            scv = ext.skip_cert_verify;
            scv.define(GetMember(json, "AllowInsecure"));

            if(method == "auto")
                method = "chacha20-ietf-poly1305";
            proxyStr = remark + " = vmess, " + hostname + ", " + port + ", " + method + ", \"" + id + "\", group=" + x.group;
            if(tlssecure)
            {
                proxyStr += ", over-tls=true, tls-host=" + host;
                if(!scv.is_undef())
                    proxyStr += ", certificate=" + std::string(scv.get() ? "0" : "1");
            }
            if(transproto == "ws")
            {
                proxyStr += ", obfs=ws, obfs-path=\"" + path + "\", obfs-header=\"Host: " + host;
                if(edge.size())
                    proxyStr += "[Rr][Nn]Edge: " + edge;
                proxyStr += "\"";
            }

            if(ext.nodelist)
                proxyStr = "vmess://" + urlsafe_base64_encode(proxyStr);
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            protocol = GetMember(json, "Protocol");
            protoparam = GetMember(json, "ProtocolParam");
            obfs = GetMember(json, "OBFS");
            obfsparam = GetMember(json, "OBFSParam");

            if(ext.nodelist)
            {
                proxyStr = "ssr://" + urlsafe_base64_encode(hostname + ":" + port + ":" + protocol + ":" + method + ":" + obfs + ":" + urlsafe_base64_encode(password) \
                           + "/?group=" + urlsafe_base64_encode(x.group) + "&remarks=" + urlsafe_base64_encode(remark) \
                           + "&obfsparam=" + urlsafe_base64_encode(obfsparam) + "&protoparam=" + urlsafe_base64_encode(protoparam));
            }
            else
            {
                proxyStr = remark + " = shadowsocksr, " + hostname + ", " + port + ", " + method + ", \"" + password + "\", group=" + x.group + ", protocol=" + protocol + ", obfs=" + obfs;
                if(protoparam.size())
                    proxyStr += ", protocol_param=" + protoparam;
                if(obfsparam.size())
                    proxyStr += ", obfs_param=" + obfsparam;
            }
            break;
        case SPEEDTEST_MESSAGE_FOUNDSS:
            plugin = GetMember(json, "Plugin");
            pluginopts = GetMember(json, "PluginOption");

            if(ext.nodelist)
            {
                proxyStr = "ss://" + urlsafe_base64_encode(method + ":" + password) + "@" + hostname + ":" + port;
                if(plugin.size() && pluginopts.size())
                {
                    proxyStr += "/?plugin=" + UrlEncode(plugin + ";" + pluginopts);
                }
                proxyStr += "&group=" + urlsafe_base64_encode(x.group) + "#" + UrlEncode(remark);
            }
            else
            {
                proxyStr = remark + " = shadowsocks, " + hostname + ", " + port + ", " + method + ", \"" + password + "\", group=" + x.group;
                if(plugin == "simple-obfs" && pluginopts.size())
                {
                    proxyStr += ", " + replace_all_distinct(pluginopts, ";", ", ");
                }
            }
            break;
        case SPEEDTEST_MESSAGE_FOUNDHTTP:
            username = GetMember(json, "Username");
            host = GetMember(json, "Host");
            tlssecure = GetMember(json, "TLSSecure") == "true";

            proxyStr = remark + " = http, upstream-proxy-address=" + hostname + ", upstream-proxy-port=" + port + ", group=" + x.group;
            if(username.size() && password.size())
                proxyStr += ", upstream-proxy-auth=true, upstream-proxy-username=" + username + ", upstream-proxy-password=" + password;
            else
                proxyStr += ", upstream-proxy-auth=false";

            if(tlssecure)
            {
                proxyStr += ", over-tls=true";
                if(host.size())
                    proxyStr += ", tls-host=" + host;
                if(!scv.is_undef())
                    proxyStr += ", certificate=" + std::string(scv.get() ? "0" : "1");
            }

            if(ext.nodelist)
                proxyStr = "http://" + urlsafe_base64_encode(proxyStr);
            break;
        case SPEEDTEST_MESSAGE_FOUNDSOCKS:
            username = GetMember(json, "Username");
            host = GetMember(json, "Host");
            tlssecure = GetMember(json, "TLSSecure") == "true";

            proxyStr = remark + " = socks, upstream-proxy-address=" + hostname + ", upstream-proxy-port=" + port + ", group=" + x.group;
            if(username.size() && password.size())
                proxyStr += ", upstream-proxy-auth=true, upstream-proxy-username=" + username + ", upstream-proxy-password=" + password;
            else
                proxyStr += ", upstream-proxy-auth=false";

            if(tlssecure)
            {
                proxyStr += ", over-tls=true";
                if(host.size())
                    proxyStr += ", tls-host=" + host;
                if(!scv.is_undef())
                    proxyStr += ", certificate=" + std::string(scv.get() ? "0" : "1");
            }

            if(ext.nodelist)
                proxyStr = "socks://" + urlsafe_base64_encode(proxyStr);
            break;
        default:
            continue;
        }

        ini.Set("{NONAME}", proxyStr);
        remarks_list.emplace_back(std::move(remark));
        nodelist.emplace_back(x);
    }

    if(ext.nodelist)
        return;

    string_array filtered_nodelist;
    ini.SetCurrentSection("POLICY");
    ini.EraseSection();

    std::string singlegroup;
    std::string name, proxies;
    string_array vArray;
    for(const std::string &x : extra_proxy_group)
    {
        eraseElements(filtered_nodelist);
        unsigned int rules_upper_bound = 0;

        vArray = split(x, "`");
        if(vArray.size() < 3)
            continue;

        rules_upper_bound = vArray.size();
        switch(hash_(vArray[1]))
        {
        case "select"_hash:
            type = "static";
            break;
        case "fallback"_hash:
            type = "static";
            if(vArray.size() < 5)
                continue;
            rules_upper_bound -= 2;
            break;
        case "url-test"_hash:
            type = "auto";
            if(vArray.size() < 5)
                continue;
            rules_upper_bound -= 2;
            break;
        case "load-balance"_hash:
            type = "balance, round-robin";
            if(vArray.size() < 5)
                continue;
            rules_upper_bound -= 2;
            break;
        case "ssid"_hash:
            {
                if(rules_upper_bound < 4)
                    continue;
                singlegroup = vArray[0] + " : wifi = " + vArray[2];
                std::string content, celluar, celluar_matcher = R"(^(.*?),?celluar\s?=\s?(.*?)(,.*)$)", rem_a, rem_b;
                for(auto iter = vArray.begin() + 3; iter != vArray.end(); iter++)
                {
                    if(regGetMatch(*iter, celluar_matcher, 4, 0, &rem_a, &celluar, &rem_b))
                    {
                        content += *iter + "\n";
                        continue;
                    }
                    content += rem_a + rem_b + "\n";
                }
                if(celluar.size())
                    singlegroup += ", celluar = " + celluar;
                singlegroup += "\n" + replace_all_distinct(trim_of(content, ','), ",", "\n");
                ini.Set("{NONAME}", base64_encode(singlegroup)); //insert order
            }
            continue;
        default:
            continue;
        }

        name = vArray[0];

        for(unsigned int i = 2; i < rules_upper_bound; i++)
            groupGenerate(vArray[i], nodelist, filtered_nodelist, true);

        if(!filtered_nodelist.size())
            filtered_nodelist.emplace_back("direct");

        if(filtered_nodelist.size() < 2) // force groups with 1 node to be static
            type = "static";

        proxies = std::accumulate(std::next(filtered_nodelist.begin()), filtered_nodelist.end(), filtered_nodelist[0], [](std::string a, std::string b)
        {
            return std::move(a) + "\n" + std::move(b);
        });

        singlegroup = name + " : " + type;
        if(type == "static")
            singlegroup += ", " + filtered_nodelist[0];
        singlegroup += "\n" + proxies + "\n";
        ini.Set("{NONAME}", base64_encode(singlegroup));
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, -2, ext.overwrite_original_rules, std::string());
}

std::string netchToQuanX(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext)
{
    INIReader ini;
    ini.store_any_line = true;
    ini.AddDirectSaveSection("general");
    ini.AddDirectSaveSection("dns");
    ini.AddDirectSaveSection("rewrite_remote");
    ini.AddDirectSaveSection("rewrite_local");
    ini.AddDirectSaveSection("task_local");
    ini.AddDirectSaveSection("mitm");
    ini.AddDirectSaveSection("server_remote");
    if(!ext.nodelist && ini.Parse(base_conf) != 0)
    {
        writeLog(0, "QuantumultX base loader failed with error: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return std::string();
    }

    netchToQuanX(nodes, ini, ruleset_content_array, extra_proxy_group, ext);

    if(ext.nodelist)
    {
        string_array allnodes;
        std::string allLinks;
        ini.GetAll("server_local", "{NONAME}", allnodes);
        if(allnodes.size())
            allLinks = std::accumulate(std::next(allnodes.begin()), allnodes.end(), allnodes[0], [](std::string a, std::string b)
            {
                return std::move(a) + "\n" + std::move(b);
            });
        return allLinks;
    }
    return ini.ToString();
}

void netchToQuanX(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext)
{
    rapidjson::Document json;
    std::string type;
    std::string remark, hostname, port, method;
    std::string password, plugin, pluginopts;
    std::string id, transproto, host, path;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string proxyStr;
    tribool udp, tfo, scv, tls13;
    bool tlssecure;
    std::vector<nodeInfo> nodelist;
    string_array remarks_list;

    ini.SetCurrentSection("server_local");
    ini.EraseSection();
    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");

        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;

        processRemark(x.remarks, remark, remarks_list);

        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)to_int(GetMember(json, "Port")));
        method = GetMember(json, "EncryptMethod");

        udp = ext.udp;
        tfo = ext.tfo;
        scv = ext.skip_cert_verify;
        tls13 = ext.tls13;
        udp.define(GetMember(json, "EnableUDP"));
        tfo.define(GetMember(json, "EnableTFO"));
        scv.define(GetMember(json, "AllowInsecure"));
        tls13.define(GetMember(json, "TLS13"));

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDVMESS:
            id = GetMember(json, "UserID");
            transproto = GetMember(json, "TransferProtocol");
            host = GetMember(json, "Host");
            path = GetMember(json, "Path");
            tlssecure = GetMember(json, "TLSSecure") == "true";
            if(method == "auto")
                method = "chacha20-ietf-poly1305";
            proxyStr = "vmess = " + hostname + ":" + port + ", method=" + method + ", password=" + id;
            if(tlssecure && !tls13.is_undef())
                proxyStr += ", tls13=" + std::string(tls13 ? "true" : "false");
            if(transproto == "ws")
            {
                if(tlssecure)
                    proxyStr += ", obfs=wss";
                else
                    proxyStr += ", obfs=ws";
                proxyStr += ", obfs-host=" + host + ", obfs-uri=" + path;
            }
            else if(tlssecure)
                proxyStr += ", obfs=over-tls, obfs-host=" + host;
            break;
        case SPEEDTEST_MESSAGE_FOUNDSS:
            password = GetMember(json, "Password");
            plugin = GetMember(json, "Plugin");
            pluginopts = GetMember(json, "PluginOption");
            proxyStr = "shadowsocks = " + hostname + ":" + port + ", method=" + method + ", password=" + password;
            if(plugin.size())
            {
                switch(hash_(plugin))
                {
                    case "simple-obfs"_hash:
                    case "obfs-local"_hash:
                        if(pluginopts.size())
                            proxyStr += ", " + replace_all_distinct(pluginopts, ";", ", ");
                        break;
                    case "v2ray-plugin"_hash:
                        pluginopts = replace_all_distinct(pluginopts, ";", "&");
                        plugin = getUrlArg(pluginopts, "mode") == "websocket" ? "ws" : "";
                        host = getUrlArg(pluginopts, "host");
                        path = getUrlArg(pluginopts, "path");
                        tlssecure = pluginopts.find("tls") != pluginopts.npos;
                        if(tlssecure && plugin == "ws")
                        {
                            plugin += 's';
                            if(!tls13.is_undef())
                                proxyStr += ", tls13=" + std::string(tls13 ? "true" : "false");
                        }
                        proxyStr += ", obfs=" + plugin;
                        if(host.size())
                            proxyStr += ", obfs-host=" + host;
                        if(path.size())
                            proxyStr += ", obfs-uri=" + path;
                        break;
                    default: continue;
                }
            }

            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            password = GetMember(json, "Password");
            protocol = GetMember(json, "Protocol");
            protoparam = GetMember(json, "ProtocolParam");
            obfs = GetMember(json, "OBFS");
            obfsparam = GetMember(json, "OBFSParam");
            proxyStr = "shadowsocks = " + hostname + ":" + port + ", method=" + method + ", password=" + password + ", ssr-protocol=" + protocol;
            if(protoparam.size())
                proxyStr += ", ssr-protocol-param=" + protoparam;
            proxyStr += ", obfs=" + obfs;
            if(obfsparam.size())
                proxyStr += ", obfs-host=" + obfsparam;
            break;
        case SPEEDTEST_MESSAGE_FOUNDHTTP:
            id = GetMember(json, "Username");
            password = GetMember(json, "Password");
            tlssecure = GetMember(json, "TLSSecure") == "true";

            proxyStr = "http = " + hostname + ":" + port + ", username=" + (id.size() ? id : "none") + ", password=" + (password.size() ? password : "none");
            if(tlssecure)
            {
                proxyStr += ", over-tls=true";
                if(!tls13.is_undef())
                    proxyStr += ", tls13=" + std::string(tls13 ? "true" : "false");
            }
            break;
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            password = GetMember(json, "Password");
            host = GetMember(json, "Host");
            tlssecure = GetMember(json, "TLSSecure") == "true";

            proxyStr = "trojan = " + hostname + ":" + port + ", password=" + password;
            if(tlssecure)
            {
                proxyStr += ", over-tls=true, tls-host=" + host;
                if(!tls13.is_undef())
                    proxyStr += ", tls13=" + std::string(tls13 ? "true" : "false");
            }
            break;
        default:
            continue;
        }
        if(!tfo.is_undef())
            proxyStr += ", fast-open=" + tfo.get_str();
        if(!udp.is_undef())
            proxyStr += ", udp-relay=" + udp.get_str();
        if(!scv.is_undef() && (x.linkType == SPEEDTEST_MESSAGE_FOUNDHTTP || x.linkType == SPEEDTEST_MESSAGE_FOUNDTROJAN))
            proxyStr += ", tls-verification=" + scv.reverse().get_str();
        proxyStr += ", tag=" + remark;

        ini.Set("{NONAME}", proxyStr);
        remarks_list.emplace_back(std::move(remark));
        nodelist.emplace_back(x);
    }

    if(ext.nodelist)
        return;

    string_multimap original_groups;
    string_array filtered_nodelist;
    ini.SetCurrentSection("policy");
    ini.GetItems(original_groups);
    ini.EraseSection();

    std::string singlegroup;
    std::string name, proxies;
    string_array vArray;
    for(const std::string &x : extra_proxy_group)
    {
        eraseElements(filtered_nodelist);
        unsigned int rules_upper_bound = 0;

        vArray = split(x, "`");
        if(vArray.size() < 3)
            continue;

        rules_upper_bound = vArray.size();
        switch(hash_(vArray[1]))
        {
        case "select"_hash:
            type = "static";
            break;
        case "url-test"_hash:
        case "fallback"_hash:
            type = "available";
            if(rules_upper_bound < 5)
                continue;
            rules_upper_bound -= 2;
            break;
        case "load-balance"_hash:
            type = "round-robin";
            if(rules_upper_bound < 5)
                continue;
            rules_upper_bound -= 2;
            break;
        case "ssid"_hash:
            if(rules_upper_bound < 4)
                continue;
            type = "ssid";
            for(auto iter = vArray.begin() + 2; iter != vArray.end(); iter++)
                filtered_nodelist.emplace_back(replace_all_distinct(*iter, "=", ":"));
            break;
        default:
            continue;
        }

        name = vArray[0];

        if(hash_(vArray[1]) != "ssid"_hash)
        {
            for(unsigned int i = 2; i < rules_upper_bound; i++)
                groupGenerate(vArray[i], nodelist, filtered_nodelist, true);

            if(!filtered_nodelist.size())
                filtered_nodelist.emplace_back("direct");

            if(filtered_nodelist.size() < 2) // force groups with 1 node to be static
                type = "static";
        }

        auto iter = std::find_if(original_groups.begin(), original_groups.end(), [name](const string_multimap::value_type &n)
        {
            std::string groupdata = n.second;
            std::string::size_type cpos = groupdata.find(",");
            if(cpos != groupdata.npos)
                return trim(groupdata.substr(0, cpos)) == name;
            else
                return false;
        });
        if(iter != original_groups.end())
        {
            vArray = split(iter->second, ",");
            if(vArray.size() > 1)
            {
                if(trim(vArray[vArray.size() - 1]).find("img-url") == 0)
                    filtered_nodelist.emplace_back(trim(vArray[vArray.size() - 1]));
            }
        }

        proxies = std::accumulate(std::next(filtered_nodelist.begin()), filtered_nodelist.end(), filtered_nodelist[0], [](std::string a, std::string b)
        {
            return std::move(a) + ", " + std::move(b);
        });

        singlegroup = type + "=" + name + ", " + proxies;
        ini.Set("{NONAME}", singlegroup);
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, -1, ext.overwrite_original_rules, ext.managed_config_prefix);

    //process scripts
    string_multimap scripts;
    std::string content, title, url;
    const std::string pattern = "^(.*? url script-.*? )(.*?)$";
    if(ini.SectionExist("rewrite_local") && ext.quanx_dev_id.size())
    {
        ini.GetItems("rewrite_local", scripts);
        ini.EraseSection("rewrite_local");
        ini.SetCurrentSection("rewrite_local");
        for(auto &x : scripts)
        {
            title = x.first;
            if(title != "{NONAME}")
                content = title + "=" + x.second;
            else
                content = x.second;

            if(regMatch(content, pattern))
            {
                url = regReplace(content, pattern, "$2");
                if(isLink(url))
                {
                    url = ext.managed_config_prefix + "/qx-script?id=" + ext.quanx_dev_id + "&url=" + urlsafe_base64_encode(url);
                    content = regReplace(content, pattern, "$1") + url;
                }
            }
            ini.Set("{NONAME}", content);
        }
    }
    eraseElements(scripts);
    string_size pos;
    if(ini.SectionExist("rewrite_remote") && ext.quanx_dev_id.size())
    {
        ini.GetItems("rewrite_remote", scripts);
        ini.EraseSection("rewrite_remote");
        ini.SetCurrentSection("rewrite_remote");
        for(auto &x : scripts)
        {
            title = x.first;
            if(title != "{NONAME}")
                content = title + "=" + x.second;
            else
                content = x.second;

            if(isLink(content))
            {
                pos = content.find(",");
                url = ext.managed_config_prefix + "/qx-rewrite?id=" + ext.quanx_dev_id + "&url=" + urlsafe_base64_encode(content.substr(0, pos));
                if(pos != content.npos)
                    url += content.substr(pos);
                content = url;
            }
            ini.Set("{NONAME}", content);
        }
    }
}

std::string netchToSSD(std::vector<nodeInfo> &nodes, std::string &group, std::string &userinfo, const extra_settings &ext)
{
    rapidjson::Document json;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    std::string hostname, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    int port, index = 0;

    if(!group.size())
        group = "SSD";

    writer.StartObject();
    writer.Key("airport");
    writer.String(group.data());
    writer.Key("port");
    writer.Int(1);
    writer.Key("encryption");
    writer.String("aes-128-gcm");
    writer.Key("password");
    writer.String("password");
    if(userinfo.size())
    {
        std::string data = replace_all_distinct(userinfo, "; ", "&");
        std::string upload = getUrlArg(data, "upload"), download = getUrlArg(data, "download"), total = getUrlArg(data, "total"), expiry = getUrlArg(data, "expire");
        double used = (to_number(upload, 0.0) + to_number(download, 0.0)) / std::pow(1024, 3) * 1.0, tot = to_number(total, 0.0) / std::pow(1024, 3) * 1.0;
        writer.Key("traffic_used");
        writer.Double(used);
        writer.Key("traffic_total");
        writer.Double(tot);
        if(expiry.size())
        {
            const time_t rawtime = to_int(expiry);
            char buffer[30];
            struct tm *dt = localtime(&rawtime);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", dt);
            writer.Key("expiry");
            writer.String(buffer);
        }
    }
    writer.Key("servers");
    writer.StartArray();

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());

        hostname = GetMember(json, "Hostname");
        port = (unsigned short)to_int(GetMember(json, "Port"));
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");
        plugin = GetMember(json, "Plugin");
        pluginopts = GetMember(json, "PluginOption");
        protocol = GetMember(json, "Protocol");
        protoparam = GetMember(json, "ProtocolParam");
        obfs = GetMember(json, "OBFS");
        obfsparam = GetMember(json, "OBFSParam");

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDSS:
            if(plugin == "obfs-local")
                plugin = "simple-obfs";
            writer.StartObject();
            writer.Key("server");
            writer.String(hostname.data());
            writer.Key("port");
            writer.Int(port);
            writer.Key("encryption");
            writer.String(method.data());
            writer.Key("password");
            writer.String(password.data());
            writer.Key("plugin");
            writer.String(plugin.data());
            writer.Key("plugin_options");
            writer.String(pluginopts.data());
            writer.Key("remarks");
            writer.String(x.remarks.data());
            writer.Key("id");
            writer.Int(index);
            writer.EndObject();
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            if(std::count(ss_ciphers.begin(), ss_ciphers.end(), method) > 0 && protocol == "origin" && obfs == "plain")
            {
                writer.StartObject();
                writer.Key("server");
                writer.String(hostname.data());
                writer.Key("port");
                writer.Int(port);
                writer.Key("encryption");
                writer.String(method.data());
                writer.Key("password");
                writer.String(password.data());
                writer.Key("remarks");
                writer.String(x.remarks.data());
                writer.Key("id");
                writer.Int(index);
                writer.EndObject();
                break;
            }
            else
                continue;
        default:
            continue;
        }
        index++;
    }
    writer.EndArray();
    writer.EndObject();
    return "ssd://" + base64_encode(sb.GetString());
}

std::string netchToMellow(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext)
{
    INIReader ini;
    ini.store_any_line = true;
    if(ini.Parse(base_conf) != 0)
    {
        writeLog(0, "Mellow base loader failed with error: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return std::string();
    }

    netchToMellow(nodes, ini, ruleset_content_array, extra_proxy_group, ext);

    return ini.ToString();
}

void netchToMellow(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext)
{
    rapidjson::Document json;
    std::string proxy;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string id, aid, transproto, faketype, host, path, quicsecure, quicsecret, tlssecure;
    std::string url;
    tribool tfo, scv;
    std::vector<nodeInfo> nodelist;
    string_array vArray, remarks_list, filtered_nodelist;

    ini.SetCurrentSection("Endpoint");

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");

        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;

        processRemark(x.remarks, remark, remarks_list);

        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)to_int(GetMember(json, "Port")));
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");

        tfo = ext.tfo;
        scv = ext.skip_cert_verify;
        tfo.define(GetMember(json, "EnableTFO"));
        scv.define(GetMember(json, "AllowInsecure"));

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDSS:
            if(GetMember(json, "Plugin").size())
                continue;
            proxy = remark + ", ss, ss://" + urlsafe_base64_encode(method + ":" + password) + "@" + hostname + ":" + port;
            break;
        case SPEEDTEST_MESSAGE_FOUNDVMESS:
            id = GetMember(json, "UserID");
            aid = GetMember(json, "AlterID");
            transproto = GetMember(json, "TransferProtocol");
            host = GetMember(json, "Host");
            path = GetMember(json, "Path");
            faketype = GetMember(json, "FakeType");
            tlssecure = GetMember(json, "TLSSecure");

            proxy = remark + ", vmess1, vmess1://" + id + "@" + hostname + ":" + port;
            if(path.size())
                proxy += path;
            proxy += "?network=" + transproto;
            switch(hash_(transproto))
            {
            case "ws"_hash:
                proxy += "&ws.host=" + UrlEncode(host);
                break;
            case "http"_hash:
                if(!host.empty())
                    proxy += "&http.host=" + UrlEncode(host);
                break;
            case "quic"_hash:
                quicsecure = GetMember(json, "QUICSecure");
                quicsecret = GetMember(json, "QUICSecret");
                if(!quicsecure.empty())
                    proxy += "&quic.security=" + quicsecure + "&quic.key=" + quicsecret;
                break;
            case "kcp"_hash:
                break;
            case "tcp"_hash:
                break;
            }
            proxy += "&tls=" + tlssecure;
            if(tlssecure == "true")
            {
                if(!host.empty())
                    proxy += "&tls.servername=" + UrlEncode(host);
            }
            if(!scv.is_undef())
                proxy += "&tls.allowinsecure=" + scv.get_str();
            if(!tfo.is_undef())
                proxy += "&sockopt.tcpfastopen=" + tfo.get_str();
            break;
        case SPEEDTEST_MESSAGE_FOUNDSOCKS:
            proxy = remark + ", builtin, socks, address=" + hostname + ", port=" + port + ", user=" + username + ", pass=" + password;
            break;
        case SPEEDTEST_MESSAGE_FOUNDHTTP:
            proxy = remark + ", builtin, http, address=" + hostname + ", port=" + port + ", user=" + username + ", pass=" + password;
            break;
        default:
            continue;
        }

        ini.Set("{NONAME}", proxy);
        remarks_list.emplace_back(std::move(remark));
        nodelist.emplace_back(x);
    }

    ini.SetCurrentSection("EndpointGroup");

    for(const std::string &x : extra_proxy_group)
    {
        eraseElements(filtered_nodelist);
        unsigned int rules_upper_bound = 0;
        url.clear();
        proxy.clear();

        vArray = split(x, "`");
        if(vArray.size() < 3)
            continue;

        rules_upper_bound = vArray.size();
        switch(hash_(vArray[1]))
        {
        case "select"_hash:
            break;
        case "url-test"_hash:
        case "fallback"_hash:
        case "load-balance"_hash:
            if(vArray.size() < 5)
                continue;
            rules_upper_bound -= 2;
            url = vArray[vArray.size() - 2];
            break;
        default:
            continue;
        }

        for(unsigned int i = 2; i < rules_upper_bound; i++)
            groupGenerate(vArray[i], nodelist, filtered_nodelist, false);

        if(!filtered_nodelist.size())
        {
            if(!remarks_list.size())
                filtered_nodelist.emplace_back("DIRECT");
            else
                filtered_nodelist = remarks_list;
        }

        //don't process these for now
        /*
        proxy = vArray[1];
        for(std::string &x : filtered_nodelist)
            proxy += "," + x;
        if(vArray[1] == "url-test" || vArray[1] == "fallback" || vArray[1] == "load-balance")
            proxy += ",url=" + url;
        */

        proxy = vArray[0] + ", ";
        /*
        for(std::string &y : filtered_nodelist)
            proxy += y + ":";
        proxy = proxy.substr(0, proxy.size() - 1);
        */
        proxy += std::accumulate(std::next(filtered_nodelist.begin()), filtered_nodelist.end(), filtered_nodelist[0], [](std::string a, std::string b)
        {
            return std::move(a) + ":" + std::move(b);
        });
        proxy += ", latency, interval=300, timeout=6"; //use hard-coded values for now

        ini.Set("{NONAME}", proxy); //insert order
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, 0, ext.overwrite_original_rules, std::string());
}

std::string netchToLoon(std::vector<nodeInfo> &nodes, const std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, const string_array &extra_proxy_group, const extra_settings &ext)
{
    rapidjson::Document json;
    INIReader ini;
    std::string proxy;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, edge, path, quicsecure, quicsecret;
    std::string output_nodelist;
    tribool scv;
    std::vector<nodeInfo> nodelist;
    bool tlssecure;
    //group pref
    std::string url;
    int interval = 0;

    string_array vArray, remarks_list, filtered_nodelist;

    ini.store_any_line = true;
    if(ini.Parse(base_conf) != INIREADER_EXCEPTION_NONE && !ext.nodelist)
    {
        writeLog(0, "Loon base loader failed with error: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return std::string();
    }

    ini.SetCurrentSection("Proxy");
    ini.EraseSection();

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");

        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;

        processRemark(x.remarks, remark, remarks_list);

        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)to_int(GetMember(json, "Port")));
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");

        scv = GetMember(json, "AllowInsecure");
        scv.define(ext.skip_cert_verify);

        proxy.clear();

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDSS:
            plugin = GetMember(json, "Plugin");
            pluginopts = GetMember(json, "PluginOption");

            proxy = "Shadowsocks," + hostname + "," + port + "," + method + ",\"" + password + "\"";
            if(plugin == "simple-obfs" || plugin == "obfs-local")
            {
                if(pluginopts.size())
                    proxy += "," + replace_all_distinct(replace_all_distinct(pluginopts, ";obfs-host=", ","), "obfs=", "");
            }
            else if(plugin.size())
                continue;
            break;
        case SPEEDTEST_MESSAGE_FOUNDVMESS:
            id = GetMember(json, "UserID");
            aid = GetMember(json, "AlterID");
            transproto = GetMember(json, "TransferProtocol");
            host = GetMember(json, "Host");
            edge = GetMember(json, "Edge");
            path = GetMember(json, "Path");
            tlssecure = GetMember(json, "TLSSecure") == "true";
            if(method == "auto")
                method = "chacha20-ietf-poly1305";

            proxy = "vmess," + hostname + "," + port + "," + method + ",\"" + id + "\",over-tls:" + (tlssecure ? "true" : "false");
            if(tlssecure)
                proxy += ",tls-name:" + host;
            switch(hash_(transproto))
            {
            case "tcp"_hash:
                proxy += ",transport:tcp";
                break;
            case "ws"_hash:
                proxy += ",transport:ws,path:" + path + ",host:" + host;
                break;
            default:
                continue;
            }
            if(!scv.is_undef())
                proxy += ",skip-cert-verify:" + std::string(scv.get() ? "1" : "0");
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            protocol = GetMember(json, "Protocol");
            protoparam = GetMember(json, "ProtocolParam");
            obfs = GetMember(json, "OBFS");
            obfsparam = GetMember(json, "OBFSParam");
            proxy = "ShadowsocksR," + hostname + "," + port + "," + method + ",\"" + password + "\"," + protocol + ",{" + protoparam + "}," + obfs + ",{" + obfsparam + "}";
            break;
        /*
        case SPEEDTEST_MESSAGE_FOUNDSOCKS:
            proxy = "socks5, " + hostname + ", " + port + ", " + username + ", " + password;
            if(ext.skip_cert_verify)
                proxy += ", skip-cert-verify:1";
            break;
        */
        case SPEEDTEST_MESSAGE_FOUNDHTTP:
            proxy = "http," + hostname + "," + port + "," + username + "," + password;
            break;
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            host = GetMember(json, "Host");
            proxy = "trojan," + hostname + "," + port + "," + password;
            if(host.size())
                proxy += ",tls-name:" + host;
            if(!scv.is_undef())
                proxy += ",skip-cert-verify:" + std::string(scv.get() ? "1" : "0");
            break;
        default:
            continue;
        }

        /*
        if(ext.tfo)
            proxy += ", tfo=true";
        if(ext.udp)
            proxy += ", udp-relay=true";
        */

        if(ext.nodelist)
            output_nodelist += remark + " = " + proxy + "\n";
        else
        {
            ini.Set("{NONAME}", remark + " = " + proxy);
            nodelist.emplace_back(x);
            remarks_list.emplace_back(std::move(remark));
        }
    }

    if(ext.nodelist)
        return output_nodelist;

    ini.SetCurrentSection("Proxy Group");
    ini.EraseSection();
    for(const std::string &x : extra_proxy_group)
    {
        eraseElements(filtered_nodelist);
        unsigned int rules_upper_bound = 0;
        url.clear();
        proxy.clear();

        vArray = split(x, "`");
        if(vArray.size() < 3)
            continue;

        rules_upper_bound = vArray.size();
        switch(hash_(vArray[1]))
        {
        case "select"_hash:
            break;
        case "url-test"_hash:
        case "fallback"_hash:
            if(vArray.size() < 5)
                continue;
            rules_upper_bound -= 2;
            url = vArray[rules_upper_bound];
            parseGroupTimes(vArray[rules_upper_bound + 1], &interval, NULL, NULL);
            break;
        case "ssid"_hash:
            if(vArray.size() < 4)
                continue;
            proxy = vArray[1] + ",default=" + vArray[2] + ",";
            proxy += std::accumulate(vArray.begin() + 4, vArray.end(), vArray[3], [](std::string a, std::string b)
            {
                return std::move(a) + "," + std::move(b);
            });
            ini.Set("{NONAME}", vArray[0] + " = " + proxy); //insert order
            continue;
        default:
            continue;
        }

        for(unsigned int i = 2; i < rules_upper_bound; i++)
            groupGenerate(vArray[i], nodelist, filtered_nodelist, true);

        if(!filtered_nodelist.size())
            filtered_nodelist.emplace_back("DIRECT");

        proxy = vArray[1] + ",";
        /*
        for(std::string &y : filtered_nodelist)
            proxy += "," + y;
        */
        proxy += std::accumulate(std::next(filtered_nodelist.cbegin()), filtered_nodelist.cend(), filtered_nodelist[0], [](std::string a, std::string b)
        {
            return std::move(a) + "," + std::move(b);
        });
        if(vArray[1] == "url-test" || vArray[1] == "fallback")
            proxy += ",url=" + url + ",interval=" + std::to_string(interval);

        ini.Set("{NONAME}", vArray[0] + " = " + proxy); //insert order
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, -4, ext.overwrite_original_rules, ext.managed_config_prefix);

    return ini.ToString();
}
