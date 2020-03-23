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

#include <algorithm>
#include <iostream>
#include <numeric>
#include <cmath>
#include <climits>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <yaml-cpp/yaml.h>

extern bool api_mode;
extern string_array ss_ciphers, ssr_ciphers;

string_array clashr_protocols = {"auth_aes128_md5", "auth_aes128_sha1"};
string_array clashr_obfs = {"plain", "http_simple", "http_post", "tls1.2_ticket_auth"};

template <typename T> T safe_as (const YAML::Node& node)
{
    if(node.IsDefined() && !node.IsNull())
        return node.as<T>();
    return T();
};

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

    for(cur = retAddrInfo; cur != NULL; cur=cur->ai_next)
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

std::string vmessConstruct(std::string add, std::string port, std::string type, std::string id, std::string aid, std::string net, std::string cipher, std::string path, std::string host, std::string edge, std::string tls, int local_port)
{
    if(!path.size())
        path = "/";
    if(!host.size())
        host = add;
    if(!id.size())
        id = "00000000-0000-0000-0000-000000000000"; //fill this field for node with empty id
    host = trim(host);
    path = trim(path);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("VMess");
    writer.Key("Remark");
    writer.String(std::string(add + ":" + port).data());
    writer.Key("Hostname");
    writer.String(add.data());
    writer.Key("Port");
    writer.Int(shortAssemble((unsigned short)to_int(port), (unsigned short)local_port));
    writer.Key("UserID");
    writer.String(id.data());
    writer.Key("AlterID");
    writer.Int(to_int(aid));
    writer.Key("EncryptMethod");
    writer.String(cipher.data());
    writer.Key("TransferProtocol");
    writer.String(net.data());
    writer.Key("Host");
    writer.String(host.data());
    writer.Key("Edge");
    writer.String(edge.data());
    if(net == "ws")
    {
        writer.Key("Path");
        writer.String(path.data());
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
    writer.EndObject();
    return sb.GetString();
}

std::string ssrConstruct(std::string group, std::string remarks, std::string remarks_base64, std::string server, std::string port, std::string protocol, std::string method, std::string obfs, std::string password, std::string obfsparam, std::string protoparam, int local_port, bool libev)
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
    writer.Int(shortAssemble((unsigned short)to_int(port), (unsigned short)local_port));
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
    writer.EndObject();
    return sb.GetString();
}

std::string ssConstruct(std::string server, std::string port, std::string password, std::string method, std::string plugin, std::string pluginopts, std::string remarks, int local_port, bool libev)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("SS");
    writer.Key("Remark");
    writer.String(remarks.data());
    writer.Key("Hostname");
    writer.String(server.data());
    writer.Key("Port");
    writer.Int(shortAssemble((unsigned short)to_int(port), (unsigned short)local_port));
    writer.Key("Password");
    writer.String(password.data());
    writer.Key("EncryptMethod");
    writer.String(method.data());
    writer.Key("Plugin");
    writer.String(plugin.data());
    writer.Key("PluginOption");
    writer.String(pluginopts.data());
    writer.EndObject();
    return sb.GetString();
}

std::string socksConstruct(std::string remarks, std::string server, std::string port, std::string username, std::string password)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("Socks5");
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
    writer.EndObject();
    return sb.GetString();
}

std::string httpConstruct(std::string remarks, std::string server, std::string port, std::string username, std::string password, bool tls)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String(tls ? "HTTPS" : "HTTP");
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
    writer.EndObject();
    return sb.GetString();
}

std::string trojanConstruct(std::string remarks, std::string server, std::string port, std::string password, std::string host, bool tlssecure)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("Type");
    writer.String("Trojan");
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
    writer.EndObject();
    return sb.GetString();
}

std::string vmessLinkConstruct(std::string remarks, std::string add, std::string port, std::string type, std::string id, std::string aid, std::string net, std::string path, std::string host, std::string tls)
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
    writer.String(type.data());
    writer.Key("id");
    writer.String(id.data());
    writer.Key("aid");
    writer.Int(to_int(aid));
    writer.Key("net");
    writer.String(net.data());
    writer.Key("path");
    writer.String(path.data());
    writer.Key("host");
    writer.String(host.data());
    writer.Key("tls");
    writer.String(tls.data());
    writer.EndObject();
    return sb.GetString();
}

bool matchRange(std::string &range, int target)
{
    string_array vArray = split(range, ",");
    bool match = false;
    int range_begin = 0, range_end = 0;
    const std::string reg_num = "\\d+", reg_range = "(\\d+)-(\\d+)", reg_not = "\\!(\\d+)", reg_not_range = "\\!(\\d+)-(\\d+)", reg_less = "(\\d+)-", reg_more = "(\\d+)\\+";
    for(std::string &x : vArray)
    {
        if(regMatch(x, reg_num))
        {
            if(to_int(x, INT_MAX) == target)
                match = true;
        }
        else if(regMatch(x, reg_range))
        {
            range_begin = to_int(regReplace(x, reg_range, "$1"), INT_MAX);
            range_end = to_int(regReplace(x, reg_range, "$2"), INT_MIN);
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
            range_begin = to_int(regReplace(x, reg_range, "$1"), INT_MAX);
            range_end = to_int(regReplace(x, reg_range, "$2"), INT_MIN);
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

std::string nodeRename(std::string remark, int groupID, const string_array &rename_array)
{
    string_array vArray;
    std::string targetRange;
    string_size pos;

    for(const std::string &x : rename_array)
    {
        targetRange = std::to_string(groupID);
        vArray = split(x, "@");
        if(vArray.size() == 1)
        {
            vArray.emplace_back("");
        }
        else if(vArray.size() != 2)
            continue;
        if(startsWith(vArray[0], "!!GROUPID="))
        {
            pos = vArray[0].find("!!", vArray[0].find("!!") + 2);
            if(pos != vArray[0].npos)
            {
                targetRange = vArray[0].substr(10, pos - 10);
                vArray[0] = vArray[0].substr(pos + 2);
            }
            else
                continue;
        }
        if(matchRange(targetRange, groupID))
            remark = regReplace(remark, vArray[0], vArray[1]);
    }
    return remark;
}

std::string removeEmoji(std::string remark)
{
    char emoji_id[2] = {(char)-16, (char)-97};
    while(true)
    {
        if(remark[0] == emoji_id[0] && remark[1] == emoji_id[1])
            remark.erase(0, 4);
        else
            break;
    }
    return remark;
}

std::string addEmoji(std::string remark, int groupID, const string_array &emoji_array)
{
    string_array vArray;
    std::string targetRange;
    string_size pos;

    for(const std::string &x : emoji_array)
    {
        targetRange = std::to_string(groupID);
        vArray = split(x, ",");
        if(vArray.size() != 2)
            continue;
        if(startsWith(vArray[0], "!!GROUPID="))
        {
            pos = vArray[0].find("!!", vArray[0].find("!!") + 2);
            if(pos != vArray[0].npos)
            {
                targetRange = vArray[0].substr(10, pos - 10);
                vArray[0] = vArray[0].substr(pos + 2);
            }
            else
                continue;
        }
        if(matchRange(targetRange, groupID) && regFind(remark, vArray[0]))
        {
            remark = vArray[1] + " " + remark;
            break;
        }
    }
    return remark;
}

void rulesetToClash(YAML::Node &base_rule, std::vector<ruleset_content> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name)
{
    string_array allRules, vArray;
    std::string rule_group, retrived_rules, strLine;
    std::stringstream strStrm;
    const std::string field_name = new_field_name ? "rules" : "Rule";
    YAML::Node Rules;

    if(!overwrite_original_rules && base_rule[field_name].IsDefined())
        Rules = base_rule[field_name];

    for(ruleset_content &x : ruleset_content_array)
    {
        rule_group = x.rule_group;
        retrived_rules = x.rule_content.get();
        if(retrived_rules.find("[]") == 0)
        {
            strLine = retrived_rules.substr(2);
            if(strLine.find("FINAL") == 0)
                strLine.replace(0, 5, "MATCH");
            strLine += "," + rule_group;
            if(std::count(strLine.begin(), strLine.end(), ',') > 2)
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            allRules.emplace_back(strLine);
            continue;
        }
        char delimiter = count(retrived_rules.begin(), retrived_rules.end(), '\n') < 1 ? '\r' : '\n';

        strStrm.clear();
        strStrm<<retrived_rules;
        std::string::size_type lineSize;
        while(getline(strStrm, strLine, delimiter))
        {
            lineSize = strLine.size();
            /*
            if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
            {
                strLine.erase(lineSize - 1);
                lineSize--;
            }
            */
            if(lineSize)
            {
                strLine = regTrim(strLine);
                lineSize = strLine.size();
            }
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            if(strLine.find("USER-AGENT") == 0 || strLine.find("URL-REGEX") == 0 || strLine.find("PROCESS-NAME") == 0 || strLine.find("AND") == 0 || strLine.find("OR") == 0) //remove unsupported types
                continue;
            /*
            if(strLine.find("IP-CIDR") == 0)
                strLine = replace_all_distinct(strLine, ",no-resolve", "");
            else if(strLine.find("DOMAIN-SUFFIX") == 0)
                strLine = replace_all_distinct(strLine, ",force-remote-dns", "");
            */
            strLine += "," + rule_group;
            if(std::count(strLine.begin(), strLine.end(), ',') > 2)
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            allRules.emplace_back(strLine);
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
    string_array allRules, vArray;
    std::string rule_group, retrived_rules, strLine;
    std::stringstream strStrm;
    const std::string field_name = new_field_name ? "rules" : "Rule";
    std::string output_content = "\n" + field_name + ":\n";

    if(!overwrite_original_rules && base_rule[field_name].IsDefined())
    {
        for(size_t i = 0; i < base_rule[field_name].size(); i++)
            output_content += " - " + safe_as<std::string>(base_rule[field_name][i]) + "\n";
    }
    base_rule.remove(field_name);

    for(ruleset_content &x : ruleset_content_array)
    {
        rule_group = x.rule_group;
        retrived_rules = x.rule_content.get();
        if(retrived_rules.find("[]") == 0)
        {
            strLine = retrived_rules.substr(2);
            if(strLine.find("FINAL") == 0)
                strLine.replace(0, 5, "MATCH");
            strLine += "," + rule_group;
            if(std::count(strLine.begin(), strLine.end(), ',') > 2)
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            output_content += " - " + strLine + "\n";
            continue;
        }
        char delimiter = count(retrived_rules.begin(), retrived_rules.end(), '\n') < 1 ? '\r' : '\n';

        strStrm.clear();
        strStrm<<retrived_rules;
        std::string::size_type lineSize;
        while(getline(strStrm, strLine, delimiter))
        {
            lineSize = strLine.size();
            if(lineSize)
            {
                strLine = regTrim(strLine);
                lineSize = strLine.size();
            }
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            if(strLine.find("USER-AGENT") == 0 || strLine.find("URL-REGEX") == 0 || strLine.find("PROCESS-NAME") == 0 || strLine.find("AND") == 0 || strLine.find("OR") == 0) //remove unsupported types
                continue;
            strLine += "," + rule_group;
            if(std::count(strLine.begin(), strLine.end(), ',') > 2)
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            output_content += " - " + strLine + "\n";
        }
    }
    return output_content;
}

void rulesetToSurge(INIReader &base_rule, std::vector<ruleset_content> &ruleset_content_array, int surge_ver, bool overwrite_original_rules, std::string remote_path_prefix)
{
    string_array allRules;
    std::string rule_group, rule_path, retrived_rules, strLine;
    std::stringstream strStrm;

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
        base_rule.EraseSection();

    const std::string rule_match_regex = "^(.*?,.*?)(,.*)(,.*)$";

    for(ruleset_content &x : ruleset_content_array)
    {
        rule_group = x.rule_group;
        retrived_rules = x.rule_content.get();
        if(retrived_rules.find("[]") == 0)
        {
            strLine = retrived_rules.substr(2);
            if(strLine == "MATCH")
                strLine = "FINAL";
            strLine += "," + rule_group;
            if(surge_ver == -1 || surge_ver == -2)
            {
                if(std::count(strLine.begin(), strLine.end(), ',') > 2 && regReplace(strLine, rule_match_regex, "$2") == ",no-resolve")
                    strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                else
                    strLine = regReplace(strLine, rule_match_regex, "$1$3");
            }
            else
            {
                if(std::count(strLine.begin(), strLine.end(), ',') > 2)
                    strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
            }
            strLine = replace_all_distinct(strLine, ",,", ",");
            allRules.emplace_back(strLine);
            continue;
        }
        else
        {
            rule_path = x.rule_path;
            if(!fileExist(rule_path))
            {
                if(surge_ver > 2)
                {
                    strLine = "RULE-SET," + rule_path + "," + rule_group;
                    allRules.emplace_back(strLine);
                    continue;
                }
                else if(surge_ver == -1 && remote_path_prefix.size())
                {
                    strLine = remote_path_prefix + "/getruleset?type=2&url=" + urlsafe_base64_encode(rule_path) + "&group=" + urlsafe_base64_encode(rule_group);
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
            {
                if(surge_ver > 2 && remote_path_prefix.size())
                {
                    strLine = "RULE-SET," + remote_path_prefix + "/getruleset?type=1&url=" + urlsafe_base64_encode(rule_path) + "," + rule_group;
                    allRules.emplace_back(strLine);
                    continue;
                }
                else if(surge_ver == -1 && remote_path_prefix.size())
                {
                    strLine = remote_path_prefix + "/getruleset?type=2&url=" + urlsafe_base64_encode(rule_path) + "&group=" + urlsafe_base64_encode(rule_group);
                    strLine += ", tag=" + rule_group + ", enabled=true";
                    base_rule.Set("filter_remote", "{NONAME}", strLine);
                    continue;
                }
                else if(surge_ver == -4 && remote_path_prefix.size())
                {
                    strLine = remote_path_prefix + "/getruleset?type=1&url=" + urlsafe_base64_encode(rule_path) + "," + rule_group;
                    base_rule.Set("Remote Rule", "{NONAME}", strLine);
                    continue;
                }
            }

            char delimiter = count(retrived_rules.begin(), retrived_rules.end(), '\n') < 1 ? '\r' : '\n';

            strStrm.clear();
            strStrm<<retrived_rules;
            std::string::size_type lineSize;
            while(getline(strStrm, strLine, delimiter))
            {
                lineSize = strLine.size();
                /*
                if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
                {
                    strLine.erase(lineSize - 1);
                    lineSize--;
                }
                */
                if(lineSize)
                {
                    strLine = regTrim(strLine);
                    lineSize = strLine.size();
                }
                if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                    continue;
                if((surge_ver == -1 || surge_ver == -2) && (strLine.find("IP-CIDR6") == 0 || strLine.find("URL-REGEX") == 0 || strLine.find("PROCESS-NAME") == 0 || strLine.find("AND") == 0 || strLine.find("OR") == 0)) //remove unsupported types
                    continue;

                strLine += "," + rule_group;
                if(surge_ver == -1 || surge_ver == -2)
                {
                    if(std::count(strLine.begin(), strLine.end(), ',') > 2 && regReplace(strLine, rule_match_regex, "$2") == ",no-resolve")
                        strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                    else
                        strLine = regReplace(strLine, rule_match_regex, "$1$3");
                }
                else
                {
                    if(std::count(strLine.begin(), strLine.end(), ',') > 2)
                        strLine = regReplace(strLine, rule_match_regex, "$1$3$2");
                }
                allRules.emplace_back(strLine);
            }
        }
    }

    for(std::string &x : allRules)
    {
        base_rule.Set("{NONAME}", x);
    }
}

void groupGenerate(std::string &rule, std::vector<nodeInfo> &nodelist, std::vector<std::string> &filtered_nodelist, bool add_direct)
{
    std::string group;
    if(rule.find("[]") == 0 && add_direct)
    {
        filtered_nodelist.emplace_back(rule.substr(2));
    }
    else if(rule.find("!!GROUP=") == 0)
    {
        if(rule.find("!!", rule.find("!!") + 2) != rule.npos)
        {
            group = rule.substr(8, rule.find("!!", rule.find("!!") + 2));
            rule = rule.substr(rule.find("!!", rule.find("!!") + 2) + 2);

            for(nodeInfo &y : nodelist)
            {
                if(regFind(y.group, group) && regFind(y.remarks, rule) && std::find(filtered_nodelist.begin(), filtered_nodelist.end(), y.remarks) == filtered_nodelist.end())
                    filtered_nodelist.emplace_back(y.remarks);
            }
        }
        else
        {
            group = rule.substr(8);

            for(nodeInfo &y : nodelist)
            {
                if(regFind(y.group, group) && std::find(filtered_nodelist.begin(), filtered_nodelist.end(), y.remarks) == filtered_nodelist.end())
                    filtered_nodelist.emplace_back(y.remarks);
            }
        }
    }
    else if(rule.find("!!GROUPID=") == 0)
    {
        if(rule.find("!!", rule.find("!!") + 2) != rule.npos)
        {
            group = rule.substr(10, rule.find("!!", rule.find("!!") + 2) - 10);
            rule = rule.substr(rule.find("!!", rule.find("!!") + 2) + 2);

            for(nodeInfo &y : nodelist)
            {
                if(matchRange(group, y.groupID) && regFind(y.remarks, rule) && std::find(filtered_nodelist.begin(), filtered_nodelist.end(), y.remarks) == filtered_nodelist.end())
                    filtered_nodelist.emplace_back(y.remarks);
            }
        }
        else
        {
            group = rule.substr(10);

            for(nodeInfo &y : nodelist)
            {
                if(matchRange(group, y.groupID) && std::find(filtered_nodelist.begin(), filtered_nodelist.end(), y.remarks) == filtered_nodelist.end())
                    filtered_nodelist.emplace_back(y.remarks);
            }
        }
    }
    else
    {
        for(nodeInfo &y : nodelist)
        {
            if(regFind(y.remarks, rule) && std::find(filtered_nodelist.begin(), filtered_nodelist.end(), y.remarks) == filtered_nodelist.end())
                filtered_nodelist.emplace_back(y.remarks);
        }
    }
}

void preprocessNodes(std::vector<nodeInfo> &nodes, extra_settings &ext)
{
    std::for_each(nodes.begin(), nodes.end(), [ext](nodeInfo &x)
    {
        x.remarks = nodeRename(x.remarks, x.groupID, ext.rename_array);
        if(ext.remove_emoji)
            x.remarks = trim(removeEmoji(x.remarks));

        if(ext.add_emoji)
            x.remarks = addEmoji(x.remarks, x.groupID, ext.emoji_array);
    });

    if(ext.sort_flag)
    {
        std::sort(nodes.begin(), nodes.end(), [](const nodeInfo &a, const nodeInfo &b)
        {
            return a.remarks < b.remarks;
        });
    }
}

void netchToClash(std::vector<nodeInfo> &nodes, YAML::Node &yamlnode, string_array &extra_proxy_group, bool clashR, extra_settings &ext)
{
    YAML::Node proxies, singleproxy, singlegroup, original_groups;
    rapidjson::Document json;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, edge, path, quicsecure, quicsecret;
    std::vector<nodeInfo> nodelist;
    bool tlssecure, replace_flag;
    string_array vArray, remarks_list, filtered_nodelist;

    for(nodeInfo &x : nodes)
    {
        singleproxy.reset();
        json.Parse(x.proxyStr.data());

        type = GetMember(json, "Type");
        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;

        while(std::count(remarks_list.begin(), remarks_list.end(), x.remarks) > 0)
            x.remarks += "$";

        remark = x.remarks;
        hostname = GetMember(json, "Hostname");
        port = GetMember(json, "Port");
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");

        singleproxy["name"] = remark;
        singleproxy["server"] = hostname;
        singleproxy["port"] = (unsigned short)stoi(port);

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
            if(std::all_of(password.begin(), password.end(), ::isdigit))
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
                if(ext.skip_cert_verify)
                    singleproxy["plugin-opts"]["skip-cert-verify"] = true;
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
            if(ext.skip_cert_verify)
                singleproxy["skip-cert-verify"] = true;
            switch(hash_(transproto))
            {
            case "tcp"_hash:
                break;
            case "ws"_hash:
                singleproxy["network"] = transproto;
                singleproxy["ws-path"] = path;
                singleproxy["ws-headers"]["Host"] = host;
                singleproxy["headers"]["Host"] = host;
                if(edge.size())
                {
                    singleproxy["ws-headers"]["Edge"] = edge;
                    singleproxy["headers"]["Edge"] = edge;
                }
                break;
            default:
                continue;
            }
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            if(!clashR)
                continue;
            //ignoring all nodes with unsupported obfs, protocols and encryption
            protocol = GetMember(json, "Protocol");
            obfs = GetMember(json, "OBFS");
            if(ext.filter_deprecated)
            {
                if(method == "chacha20" && !clashR) //the mainline core no longer supports chacha20, but clashR core still does
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
            if(std::all_of(password.begin(), password.end(), ::isdigit))
                singleproxy["password"].SetTag("str");
            singleproxy["protocol"] = protocol;
            singleproxy["protocolparam"] = protoparam;
            singleproxy["obfs"] = obfs;
            singleproxy["obfsparam"] = obfsparam;
            break;
        case SPEEDTEST_MESSAGE_FOUNDSOCKS:
            singleproxy["type"] = "socks5";
            singleproxy["username"] = username;
            singleproxy["password"] = password;
            if(std::all_of(password.begin(), password.end(), ::isdigit))
                singleproxy["password"].SetTag("str");
            if(ext.skip_cert_verify)
                singleproxy["skip-cert-verify"] = true;
            break;
        case SPEEDTEST_MESSAGE_FOUNDHTTP:
            singleproxy["type"] = "http";
            singleproxy["username"] = username;
            singleproxy["password"] = password;
            if(std::all_of(password.begin(), password.end(), ::isdigit))
                singleproxy["password"].SetTag("str");
            singleproxy["tls"] = type == "HTTPS";
            if(ext.skip_cert_verify)
                singleproxy["skip-cert-verify"] = true;
            break;
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            singleproxy["type"] = "trojan";
            singleproxy["password"] = password;
            if(std::all_of(password.begin(), password.end(), ::isdigit))
                singleproxy["password"].SetTag("str");
            if(ext.skip_cert_verify)
                singleproxy["skip-cert-verify"] = true;
            break;
        default:
            continue;
        }

        if(ext.udp)
            singleproxy["udp"] = true;
        singleproxy.SetStyle(YAML::EmitterStyle::Flow);
        proxies.push_back(singleproxy);
        remarks_list.emplace_back(remark);
        nodelist.emplace_back(x);
    }

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

    std::string groupname;

    for(std::string &x : extra_proxy_group)
    {
        singlegroup.reset();
        eraseElements(filtered_nodelist);
        replace_flag = false;
        unsigned int rules_upper_bound = 0;

        vArray = split(x, "`");
        if(vArray.size() < 3)
            continue;

        singlegroup["name"] = vArray[0];
        singlegroup["type"] = vArray[1];

        rules_upper_bound = vArray.size();
        switch(hash_(vArray[1]))
        {
        case "select"_hash:
            break;
        case "url-test"_hash:
        case "fallback"_hash:
        case "load-balance"_hash:
            if(rules_upper_bound < 5)
                continue;
            rules_upper_bound -= 2;
            singlegroup["url"] = vArray[rules_upper_bound];
            singlegroup["interval"] = to_int(vArray[rules_upper_bound + 1]);
            break;
        default:
            continue;
        }

        for(unsigned int i = 2; i < rules_upper_bound; i++)
            groupGenerate(vArray[i], nodelist, filtered_nodelist, true);

        if(!filtered_nodelist.size())
            filtered_nodelist.emplace_back("DIRECT");

        singlegroup["proxies"] = filtered_nodelist;
        //singlegroup.SetStyle(YAML::EmitterStyle::Flow);

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

std::string netchToClash(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, bool clashR, extra_settings &ext)
{
    YAML::Node yamlnode;

    try
    {
        yamlnode = YAML::Load(base_conf);
    }
    catch (std::exception &e)
    {
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

    std::string output_content = rulesetToClashStr(yamlnode, ruleset_content_array, ext.overwrite_original_rules, ext.clash_new_field_name);
    output_content.insert(0, YAML::Dump(yamlnode));

    return output_content;
}

std::string netchToSurge(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, int surge_ver, extra_settings &ext)
{
    rapidjson::Document json;
    INIReader ini;
    std::string proxy;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, edge, path, quicsecure, quicsecret;
    std::string output_nodelist;
    std::vector<nodeInfo> nodelist;
    unsigned short local_port = 1080;
    bool tlssecure;
    //group pref
    std::string url;
    int interval = 0;
    std::string ssid_default;

    string_array vArray, remarks_list, filtered_nodelist, args;

    ini.store_any_line = true;
    // filter out sections that requires direct-save
    ini.AddDirectSaveSection("Rule");
    ini.AddDirectSaveSection("MITM");
    ini.AddDirectSaveSection("Script");
    ini.AddDirectSaveSection("URL Rewrite");
    ini.AddDirectSaveSection("Header Rewrite");
    if(ini.Parse(base_conf) != 0 && !ext.nodelist)
        return std::string();

    ini.SetCurrentSection("Proxy");
    ini.EraseSection();
    ini.Set("{NONAME}", "DIRECT = direct");

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");

        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;
        remark = x.remarks;

        while(std::count(remarks_list.begin(), remarks_list.end(), x.remarks) > 0)
            x.remarks += "$";

        remark = x.remarks;
        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");
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
            if(plugin.size() && pluginopts.size())
                proxy += "," + replace_all_distinct(pluginopts, ";", ",");
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
            switch(hash_(transproto))
            {
            case "tcp"_hash:
                break;
            case "ws"_hash:
                proxy += ", ws=true, ws-path=" + path + ", ws-headers=Host:" + host;
                if(edge.size())
                    proxy += "|Edge:" + edge;
                break;
            default:
                continue;
            }
            if(ext.skip_cert_verify)
                proxy += ", skip-cert-verify=1";
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
                args.emplace_back(obfsparam);
            }
            if(protoparam.size())
            {
                args.emplace_back("-G");
                args.emplace_back(protoparam);
            }
            proxy += std::accumulate(std::next(args.begin()), args.end(), args[0], [](std::string a, std::string b)
            {
                return std::move(a) + "\", args=\"" + std::move(b);
            });
            proxy += "\", local-port=" + std::to_string(local_port) + ", addresses=" + ((isIPv4(hostname) || isIPv6(hostname)) ? hostname : hostnameToIPAddr(hostname));
            //proxy += "\", local-port=" + std::to_string(local_port);
            local_port++;
            break;
        case SPEEDTEST_MESSAGE_FOUNDSOCKS:
            proxy = "socks5, " + hostname + ", " + port + ", " + username + ", " + password;
            if(ext.skip_cert_verify)
                proxy += ", skip-cert-verify=1";
            break;
        case SPEEDTEST_MESSAGE_FOUNDHTTP:
            proxy = "http, " + hostname + ", " + port + ", " + username + ", " + password;
            proxy += std::string(", tls=") + (type == "HTTPS" ? "true" : "false");
            if(ext.skip_cert_verify)
                proxy += ", skip-cert-verify=1";
            break;
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            if(surge_ver < 4)
                continue;
            proxy = "trojan, " + hostname + ", " + port + ", password=" + password;
            if(ext.skip_cert_verify)
                proxy += ", skip-cert-verify=1";
            break;
        default:
            continue;
        }

        if(ext.tfo)
            proxy += ", tfo=true";
        if(ext.udp)
            proxy += ", udp-relay=true";

        if(ext.nodelist)
            output_nodelist += remark + " = " + proxy + "\n";
        else
        {
            ini.Set("{NONAME}", remark + " = " + proxy);
            nodelist.emplace_back(x);
            remarks_list.emplace_back(remark);
        }
    }

    if(ext.nodelist)
        return output_nodelist;

    ini.SetCurrentSection("Proxy Group");
    ini.EraseSection();
    for(std::string &x : extra_proxy_group)
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
            if(rules_upper_bound < 5)
                continue;
            rules_upper_bound -= 2;
            url = vArray[rules_upper_bound];
            interval = to_int(vArray[rules_upper_bound + 1]);
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
        else if(vArray[1] == "load-balance")
            proxy += ",url=" + url;

        ini.Set("{NONAME}", vArray[0] + " = " + proxy); //insert order
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, surge_ver, ext.overwrite_original_rules, ext.managed_config_prefix);

    return ini.ToString();
}

std::string netchToSS(std::vector<nodeInfo> &nodes, extra_settings &ext)
{
    rapidjson::Document json;
    std::string remark, hostname, port, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string proxyStr, allLinks;

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());

        remark = x.remarks;
        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
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
            proxyStr = "ss://" + urlsafe_base64_encode(method + ":" + password) + "@" + hostname + ":" + port;
            if(plugin.size() && pluginopts.size())
            {
                proxyStr += "/?plugin=" + UrlEncode(plugin + ";" +pluginopts);
            }
            proxyStr += "#" + UrlEncode(remark);
            break;
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            if(std::count(ss_ciphers.begin(), ss_ciphers.end(), method) > 0 && protocol == "origin" && obfs == "plain")
                proxyStr = "ss://" + urlsafe_base64_encode(method + ":" + password) + "@" + hostname + ":" + port + "#" + UrlEncode(remark);
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

std::string netchToSSSub(std::vector<nodeInfo> &nodes, extra_settings &ext)
{
    rapidjson::Document json;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    std::string remark, hostname, password, method;
    std::string plugin, pluginopts;
    std::string protocol, obfs;
    int port;

    writer.StartArray();
    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());

        remark = x.remarks;
        hostname = x.server;
        port = (unsigned short)stoi(GetMember(json, "Port"));
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
        writer.StartObject();
        writer.Key("server");
        writer.String(hostname.data());
        writer.Key("server_port");
        writer.Int(port);
        writer.Key("method");
        writer.String(method.data());
        writer.Key("password");
        writer.String(password.data());
        writer.Key("remarks");
        writer.String(remark.data());
        writer.Key("plugin");
        writer.String(plugin.data());
        writer.Key("plugin_opts");
        writer.String(pluginopts.data());
        writer.EndObject();
    }
    writer.EndArray();
    return sb.GetString();
}

std::string netchToSSR(std::vector<nodeInfo> &nodes, extra_settings &ext)
{
    rapidjson::Document json;
    std::string remark, hostname, port, password, method;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string proxyStr, allLinks;

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());

        remark = x.remarks;
        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");
        protocol = GetMember(json, "Protocol");
        protoparam = GetMember(json, "ProtocolParam");
        obfs = GetMember(json, "OBFS");
        obfsparam = GetMember(json, "OBFSParam");

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDSSR:
            proxyStr = "ssr://" + urlsafe_base64_encode(hostname + ":" + port + ":" + protocol + ":" + method + ":" + obfs + ":" + urlsafe_base64_encode(password) \
                       + "/?group=" + urlsafe_base64_encode(x.group) + "&remarks=" + urlsafe_base64_encode(remark) \
                       + "&obfsparam=" + urlsafe_base64_encode(obfsparam) + "&protoparam=" + urlsafe_base64_encode(protoparam));
            break;
        case SPEEDTEST_MESSAGE_FOUNDSS:
            if(std::count(ssr_ciphers.begin(), ssr_ciphers.end(), method) > 0 && !GetMember(json, "Plugin").size() && !GetMember(json, "Plugin").size())
                proxyStr = "ssr://" + urlsafe_base64_encode(hostname + ":" + port + ":origin:" + method + ":plain:" + urlsafe_base64_encode(password) \
                           + "/?group=" + urlsafe_base64_encode(x.group) + "&remarks=" + urlsafe_base64_encode(remark));
            break;
        default:
            continue;
        }
        allLinks += proxyStr + "\n";
    }

    return base64_encode(allLinks);
}

std::string netchToVMess(std::vector<nodeInfo> &nodes, extra_settings &ext)
{
    rapidjson::Document json;
    std::string remark, hostname, port, method;
    std::string id, aid, transproto, faketype, host, path, quicsecure, quicsecret;
    std::string proxyStr, allLinks;
    bool tlssecure;

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());

        remark = x.remarks;
        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
        method = GetMember(json, "EncryptMethod");
        id = GetMember(json, "UserID");
        aid = GetMember(json, "AlterID");
        transproto = GetMember(json, "TransferProtocol");
        host = GetMember(json, "Host");
        path = GetMember(json, "Path");
        faketype = GetMember(json, "FakeType");
        tlssecure = GetMember(json, "TLSSecure") == "true";

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDVMESS:
            proxyStr = "vmess://" + base64_encode(vmessLinkConstruct(remark, hostname, port, faketype, id, aid, transproto, path, host, tlssecure ? "tls" : ""));
            break;
        default:
            continue;
        }
        allLinks += proxyStr + "\n";
    }

    return base64_encode(allLinks);
}

std::string netchToTrojan(std::vector<nodeInfo> &nodes, extra_settings &ext)
{
    rapidjson::Document json;
    std::string server, port, psk, remark;
    std::string proxyStr, allLinks;

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());

        remark = x.remarks;
        server = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
        psk = GetMember(json, "Password");

        switch(x.linkType)
        {
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            proxyStr = "trojan://" + psk + "@" + server + ":" + port + "#" + UrlEncode(remark);
            break;
        default:
            continue;
        }
        allLinks += proxyStr + "\n";
    }

    return base64_encode(allLinks);
}

std::string netchToQuan(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext)
{
    INIReader ini;
    ini.store_any_line = true;
    if(!ext.nodelist && ini.Parse(base_conf) != 0)
        return std::string();

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

void netchToQuan(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext)
{
    rapidjson::Document json;
    std::string type;
    std::string remark, hostname, port, method, password;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, edge, path, quicsecure, quicsecret;
    std::string proxyStr, allLinks;
    bool tlssecure;
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

        while(std::count(remarks_list.begin(), remarks_list.end(), x.remarks) > 0)
            x.remarks += "$";

        remark = x.remarks;

        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
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

            if(method == "auto")
                method = "chacha20-ietf-poly1305";
            proxyStr = remark + " = vmess, " + hostname + ", " + port + ", " + method + ", \"" + id + "\", group=" + x.group;
            if(tlssecure)
                proxyStr += ", over-tls=true, tls-host=" + host;
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
        default:
            continue;
        }

        ini.Set("{NONAME}", proxyStr);
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
    for(std::string &x : extra_proxy_group)
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
        default:
            continue;
        }

        name = vArray[0];

        for(unsigned int i = 2; i < rules_upper_bound; i++)
            groupGenerate(vArray[i], nodelist, filtered_nodelist, true);

        if(!filtered_nodelist.size())
            filtered_nodelist.emplace_back("direct");

        proxies = std::accumulate(std::next(filtered_nodelist.begin()), filtered_nodelist.end(), filtered_nodelist[0], [](std::string a, std::string b)
        {
            return std::move(a) + "\n" + std::move(b);
        });

        if(filtered_nodelist.size() < 2) // force groups with 1 node to be static
            type = "static";

        singlegroup = name + " : " + type;
        if(type == "static")
            singlegroup += ", " + filtered_nodelist[0];
        singlegroup += "\n" + proxies + "\n";
        ini.Set("{NONAME}", base64_encode(singlegroup));
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, -2, ext.overwrite_original_rules, std::string());
}

std::string netchToQuanX(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext)
{
    INIReader ini;
    ini.store_any_line = true;
    ini.AddDirectSaveSection("rewrite_local");
    if(!ext.nodelist && ini.Parse(base_conf) != 0)
        return std::string();

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

void netchToQuanX(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext)
{
    rapidjson::Document json;
    std::string type;
    std::string remark, hostname, port, method;
    std::string password, plugin, pluginopts;
    std::string id, transproto, host, path;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string proxyStr;
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

        while(std::count(remarks_list.begin(), remarks_list.end(), x.remarks) > 0)
            x.remarks += "$";

        remark = x.remarks;

        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
        method = GetMember(json, "EncryptMethod");

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
            if(plugin.size() && pluginopts.size())
                proxyStr += ", " + replace_all_distinct(pluginopts, ";", ", ");
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
                proxyStr += ", over-tls=true";
            break;
        case SPEEDTEST_MESSAGE_FOUNDTROJAN:
            password = GetMember(json, "Password");
            host = GetMember(json, "Host");
            tlssecure = GetMember(json, "TLSSecure") == "true";

            proxyStr = "trojan = " + hostname + ":" + port + ", password=" + password;
            if(tlssecure)
            {
                proxyStr += ", over-tls=true, tls-host=" + host;
            }
            break;
        default:
            continue;
        }
        if(ext.tfo)
            proxyStr += ", fast-open=true";
        if(ext.udp)
            proxyStr += ", udp-relay=true";
        proxyStr += ", tag=" + remark;

        remarks_list.push_back(remark);
        ini.Set("{NONAME}", proxyStr);
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
    for(std::string &x : extra_proxy_group)
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
        default:
            continue;
        }

        name = vArray[0];

        for(unsigned int i = 2; i < rules_upper_bound; i++)
            groupGenerate(vArray[i], nodelist, filtered_nodelist, true);

        if(!filtered_nodelist.size())
            filtered_nodelist.emplace_back("direct");

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

        if(filtered_nodelist.size() < 2) // force groups with 1 node to be static
            type = "static";

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
                if(startsWith(url, "https://") || startsWith(url, "http://"))
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

            if(startsWith(content, "https://") || startsWith(content, "http://"))
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

std::string netchToSSD(std::vector<nodeInfo> &nodes, std::string &group, std::string &userinfo, extra_settings &ext)
{
    rapidjson::Document json;
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    std::string remark, hostname, password, method;
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

        remark = "\"" + replace_all_distinct(UTF8ToCodePoint(x.remarks), "\\u1f1", "\\ud83c\\udd") + "\""; //convert UTF-8 characters to code points
        hostname = GetMember(json, "Hostname");
        port = (unsigned short)stoi(GetMember(json, "Port"));
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
            writer.RawValue(remark.data(), remark.size(), rapidjson::Type::kStringType);
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
                writer.String(pluginopts.data());
                writer.Key("remarks");
                writer.RawValue(remark.data(), remark.size(), rapidjson::Type::kStringType);
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

std::string netchToMellow(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext)
{
    INIReader ini;
    ini.store_any_line = true;
    if(ini.Parse(base_conf) != 0)
        return std::string();

    netchToMellow(nodes, ini, ruleset_content_array, extra_proxy_group, ext);

    return ini.ToString();
}

void netchToMellow(std::vector<nodeInfo> &nodes, INIReader &ini, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext)
{
    rapidjson::Document json;
    std::string proxy;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string id, aid, transproto, faketype, host, path, quicsecure, quicsecret, tlssecure;
    std::string url;
    std::vector<nodeInfo> nodelist;
    string_array vArray, remarks_list, filtered_nodelist;

    ini.SetCurrentSection("Endpoint");

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");

        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;
        remark = x.remarks;

        while(std::count(remarks_list.begin(), remarks_list.end(), remark) > 0)
            remark = x.remarks = x.remarks + "$";
        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");

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
            if(transproto == "ws")
            {
                proxy += "&ws.host=" + UrlEncode(host);
            }
            proxy += "&tls=" + tlssecure;
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
        remarks_list.emplace_back(remark);
        nodelist.emplace_back(x);
    }

    ini.SetCurrentSection("EndpointGroup");

    for(std::string &x : extra_proxy_group)
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

std::string netchToLoon(std::vector<nodeInfo> &nodes, std::string &base_conf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, extra_settings &ext)
{
    rapidjson::Document json;
    INIReader ini;
    std::string proxy;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, edge, path, quicsecure, quicsecret;
    std::string output_nodelist;
    std::vector<nodeInfo> nodelist;
    bool tlssecure;
    //group pref
    std::string url;
    int interval = 0;
    std::string ssid_default;

    string_array vArray, remarks_list, filtered_nodelist;

    ini.store_any_line = true;
    if(ini.Parse(base_conf) != INIREADER_EXCEPTION_NONE && !ext.nodelist)
        return std::string();

    ini.SetCurrentSection("Proxy");
    ini.EraseSection();

    for(nodeInfo &x : nodes)
    {
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");

        if(ext.append_proxy_type)
            x.remarks = "[" + type + "] " + x.remarks;
        remark = x.remarks;

        while(std::count(remarks_list.begin(), remarks_list.end(), x.remarks) > 0)
            x.remarks += "$";

        remark = x.remarks;
        hostname = GetMember(json, "Hostname");
        port = std::to_string((unsigned short)stoi(GetMember(json, "Port")));
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");
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
            if(ext.skip_cert_verify)
                proxy += ",skip-cert-verify:1";
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
            remarks_list.emplace_back(remark);
        }
    }

    if(ext.nodelist)
        return output_nodelist;

    ini.SetCurrentSection("Proxy Group");
    ini.EraseSection();
    for(std::string &x : extra_proxy_group)
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
            interval = to_int(vArray[rules_upper_bound + 1]);
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

std::string buildGistData(std::string name, std::string content)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    writer.StartObject();
    writer.Key("description");
    writer.String("subconverter");
    writer.Key("public");
    writer.Bool(false);
    writer.Key("files");
    writer.StartObject();
    writer.Key(name.data());
    writer.StartObject();
    writer.Key("content");
    writer.String(content.data());
    writer.EndObject();
    writer.EndObject();
    writer.EndObject();
    return sb.GetString();
}

int uploadGist(std::string name, std::string path, std::string content, bool writeManageURL)
{
    INIReader ini;
    rapidjson::Document json;
    std::string token, id, username, retData, url;
    int retVal = 0;

    if(!fileExist("gistconf.ini"))
    {
        //std::cerr<<"gistconf.ini not found. Skipping...\n";
        writeLog(0, "gistconf.ini not found. Skipping...", LOG_LEVEL_ERROR);
        return -1;
    }

    ini.ParseFile("gistconf.ini");
    if(ini.EnterSection("common") != 0)
    {
        //std::cerr<<"gistconf.ini has incorrect format. Skipping...\n";
        writeLog(0, "gistconf.ini has incorrect format. Skipping...", LOG_LEVEL_ERROR);
        return -1;
    }

    token = ini.Get("token");
    if(!token.size())
    {
        //std::cerr<<"No token is provided. Skipping...\n";
        writeLog(0, "No token is provided. Skipping...", LOG_LEVEL_ERROR);
        return -1;
    }

    id = ini.Get("id");
    username = ini.Get("username");
    if(!path.size())
    {
        if(ini.ItemExist("path"))
            path = ini.Get(name, "path");
        else
            path = name;
    }

    if(!id.size())
    {
        //std::cerr<<"No gist id is provided. Creating new gist...\n";
        writeLog(0, "No Gist id is provided. Creating new Gist...", LOG_LEVEL_ERROR);
        retVal = webPost("https://api.github.com/gists", buildGistData(path, content), getSystemProxy(), token, &retData);
        if(retVal != 201)
        {
            //std::cerr<<"Create new Gist failed! Return data:\n"<<retData<<"\n";
            writeLog(0, "Create new Gist failed! Return data:\n" + retData, LOG_LEVEL_ERROR);
            return -1;
        }
    }
    else
    {
        url = "https://gist.githubusercontent.com/" + username + "/" + id + "/raw/" + path;
        //std::cerr<<"Gist id provided. Modifying Gist...\n";
        writeLog(0, "Gist id provided. Modifying Gist...", LOG_LEVEL_INFO);
        if(writeManageURL)
            content = "#!MANAGED-CONFIG " + url + "\n" + content;
        retVal = webPatch("https://api.github.com/gists/" + id, buildGistData(path, content), getSystemProxy(), token, &retData);
        if(retVal != 200)
        {
            //std::cerr<<"Modify gist failed! Return data:\n"<<retData<<"\n";
            writeLog(0, "Modify Gist failed! Return data:\n" + retData, LOG_LEVEL_ERROR);
            return -1;
        }
    }
    json.Parse(retData.data());
    GetMember(json, "id", id);
    if(json.HasMember("owner"))
        GetMember(json["owner"], "login", username);
    url = "https://gist.githubusercontent.com/" + username + "/" + id + "/raw/" + path;
    //std::cerr<<"Writing to Gist success!\nGenerator: "<<name<<"\nPath: "<<path<<"\nRaw URL: "<<url<<"\nGist owner: "<<username<<"\n";
    writeLog(0, "Writing to Gist success!\nGenerator: " + name + "\nPath: " + path + "\nRaw URL: " + url + "\nGist owner: " + username, LOG_LEVEL_INFO);

    ini.EraseSection();
    ini.Set("token", token);
    ini.Set("id", id);
    ini.Set("username", username);

    ini.SetCurrentSection(path);
    ini.EraseSection();
    ini.Set("type", name);
    ini.Set("url", url);

    ini.ToFile("gistconf.ini");
    return 0;
}
