#include "misc.h"
#include "speedtestutil.h"
#include "ini_reader.h"
#include "rapidjson_extra.h"
#include "yamlcpp_extra.h"
#include "webget.h"
#include "subexport.h"

#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <yaml-cpp/yaml.h>

extern bool overwrite_original_rules;
extern string_array renames, emojis;
extern bool add_emoji, remove_old_emoji;

std::string vmessConstruct(std::string add, std::string port, std::string type, std::string id, std::string aid, std::string net, std::string cipher, std::string path, std::string host, std::string tls, int local_port)
{
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
    writer.Int(shortAssemble((unsigned short)stoi(port), (unsigned short)local_port));
    writer.Key("UserID");
    writer.String(id.data());
    writer.Key("AlterID");
    writer.Int(stoi(aid));
    writer.Key("EncryptMethod");
    writer.String(cipher.data());
    writer.Key("TransferProtocol");
    writer.String(net.data());
    if(net == "ws")
    {
        writer.Key("Host");
        writer.String(host.data());
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
    writer.Int(shortAssemble((unsigned short)stoi(port), (unsigned short)local_port));
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
    writer.Int(shortAssemble((unsigned short)stoi(port), (unsigned short)local_port));
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

std::string nodeRename(std::string remark)
{
    string_array vArray;

    for(std::string &x : renames)
    {
        vArray = split(x, "@");
        if(vArray.size() == 1)
        {
            remark = regReplace(remark, vArray[0], "");
        }
        else if(vArray.size() == 2)
        {
            remark = regReplace(remark, vArray[0], vArray[1]);
        }
        else
            continue;
    }
    return remark;
}

std::string removeEmoji(std::string remark)
{
    if(!remove_old_emoji)
        return remark;
    char emoji_id[2] = {(char)-16, (char)-97};
    while(true)
    {
        if(remark[0] == emoji_id[0] && remark[1] == emoji_id[1])
            remark = remark.substr(4);
        else
            break;
    }
    return remark;
}

std::string addEmoji(std::string remark)
{
    if(!add_emoji)
        return remark;
    string_array vArray;
    for(std::string &x : emojis)
    {
        vArray = split(x, ",");
        if(vArray.size() != 2)
            continue;
        if(regFind(remark, vArray[0]))
        {
            remark = vArray[1] + " " + remark;
            break;
        }
    }
    return remark;
}

void rulesetToClash(YAML::Node &base_rule, std::vector<ruleset_content> &ruleset_content_array)
{
    string_array allRules, vArray;
    std::string rule_group, rule_path, retrived_rules, strLine;
    std::stringstream strStrm;
    YAML::Node Rules;

    if(!overwrite_original_rules)
        Rules = base_rule["Rule"];

    for(ruleset_content &x : ruleset_content_array)
    {
        rule_group = x.rule_group;
        retrived_rules = x.rule_content;
        if(retrived_rules.find("[]") == 0)
        {
            allRules.emplace_back(retrived_rules.substr(2) + "," + rule_group);
            continue;
        }
        char delimiter = count(retrived_rules.begin(), retrived_rules.end(), '\n') <= 1 ? '\r' : '\n';

        strStrm.clear();
        strStrm<<retrived_rules;
        while(getline(strStrm, strLine, delimiter))
        {
            strLine = replace_all_distinct(strLine, "\r", ""); //remove line break
            if(!strLine.size() || strLine.find("#") == 0 || strLine.find(";") == 0) //remove comments
                continue;
            if(strLine.find("USER-AGENT") == 0 || strLine.find("URL-REGEX") == 0 || strLine.find("PROCESS-NAME") == 0) //remove unsupported types
                continue;
            if(strLine.find("IP-CIDR") == 0)
                strLine = replace_all_distinct(strLine, ",no-resolve", "");
            else if(strLine.find("DOMAIN-SUFFIX") == 0)
                strLine = replace_all_distinct(strLine, ",force-remote-dns", "");
            strLine += "," + rule_group;
            allRules.emplace_back(strLine);
        }
    }

    for(std::string &x : allRules)
    {
        Rules.push_back(x);
    }
    base_rule["Rule"] = Rules;
}

std::string netchToClash(std::vector<nodeInfo> &nodes, std::string &baseConf, std::vector<ruleset_content> &ruleset_content_array, string_array &extra_proxy_group, bool clashR)
{
    YAML::Node yamlnode, proxies, singleproxy, singlegroup, original_groups;
    rapidjson::Document json;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, path, quicsecure, quicsecret;
    std::vector<std::string> nodelist;
    bool tlssecure, replace_flag;
    string_array vArray, filtered_nodelist;

    try
    {
        yamlnode = YAML::Load(baseConf);
    }
    catch (std::exception &e)
    {
        return std::string();
    }


    for(nodeInfo &x : nodes)
    {
        singleproxy.reset();
        json.Parse(x.proxyStr.data());
        type = GetMember(json, "Type");
        remark = addEmoji(trim(removeEmoji(nodeRename(x.remarks))));
        hostname = GetMember(json, "Hostname");
        port = GetMember(json, "Port");
        username = GetMember(json, "Username");
        password = GetMember(json, "Password");
        method = GetMember(json, "EncryptMethod");

        if(type == "SS")
        {
            plugin = GetMember(json, "Plugin");
            pluginopts = replace_all_distinct(GetMember(json, "PluginOption"), ";", "&");
            singleproxy["type"] = "ss";
            singleproxy["cipher"] = method;
            if(plugin == "simple-obfs" || plugin == "obfs-local")
            {
                singleproxy["plugin"] = "obfs";
                singleproxy["plugin-opts"]["mode"] = UrlDecode(getUrlArg(pluginopts, "obfs"));
                singleproxy["plugin-opts"]["host"] = UrlDecode(getUrlArg(pluginopts, "obfs-host"));
            }
        }
        else if(type == "VMess")
        {
            id = GetMember(json, "UserID");
            aid = GetMember(json, "AlterID");
            transproto = GetMember(json, "TransferProtocol");
            host = GetMember(json, "Host");
            path = GetMember(json, "Path");
            tlssecure = GetMember(json, "TLSSecure") == "true";
            singleproxy["type"] = "vmess";
            singleproxy["uuid"] = id;
            singleproxy["alterId"] = stoi(aid);
            singleproxy["cipher"] = method;
            singleproxy["tls"] = tlssecure;
            if(transproto == "ws")
            {
                singleproxy["network"] = transproto;
                singleproxy["ws-path"] = path;
                singleproxy["ws-headers"]["Host"] = host;
            }
            else if(transproto == "kcp" || transproto == "h2" || transproto == "quic")
                continue;
        }
        else if(type == "SSR" && clashR)
        {
            protocol = GetMember(json, "Protocol");
            protoparam = GetMember(json, "ProtocolParam");
            obfs = GetMember(json, "OBFS");
            obfsparam = GetMember(json, "OBFSParam");
            singleproxy["type"] = "ssr";
            singleproxy["cipher"] = method;
            singleproxy["protocol"] = protocol;
            singleproxy["protocolparam"] = protoparam;
            singleproxy["obfs"] = obfs;
            singleproxy["obfsparam"] = obfsparam;
        }
        else if(type == "Socks5")
        {
            singleproxy["type"] = "socks";
            singleproxy["username"] = username;
        }
        else if(type == "HTTP" || type == "HTTPS")
        {
            singleproxy["type"] = "http";
            singleproxy["username"] = username;
            singleproxy["tls"] = type == "HTTPS";
        }
        else
            continue;
        singleproxy["password"] = password;
        singleproxy["name"] = remark;
        singleproxy["server"] = hostname;
        singleproxy["port"] = (unsigned short)stoi(port);
        proxies.push_back(singleproxy);
        nodelist.emplace_back(remark);
    }

    yamlnode["Proxy"] = proxies;
    std::string groupname;
    /*
    if(!overwrite_original_groups)
    {
        original_groups = yamlnode["Proxy Group"];
        for(unsigned int i = 0; i < original_groups.size(); i++)
        {
            groupname = original_groups[i]["name"].as<std::string>();
            if(groupname == "Proxy" || groupname == "PROXY")
            {
                original_groups[i]["name"] = "Proxy";
                original_groups[i]["proxies"] = vectorToNode(nodelist);
                break;
            }
        }
    }
    else
    {
        singlegroup["name"] = "Proxy";
        singlegroup["type"] = "select";
        singlegroup["proxies"] = vectorToNode(nodelist);
        original_groups.push_back(singlegroup);
    }
    */

    for(std::string &x : extra_proxy_group)
    {
        singlegroup.reset();
        eraseElements(filtered_nodelist);
        replace_flag = false;
        unsigned int rules_upper_bound = 0;

        vArray = split(x, "`");
        if(vArray.size() < 3)
            continue;

        if(vArray[1] == "select")
        {
            rules_upper_bound = vArray.size();
        }
        else if(vArray[1] == "url-test" || vArray[1] == "fallback" || vArray[1] == "load-balance")
        {
            if(vArray.size() < 5)
                continue;
            rules_upper_bound = vArray.size() - 2;
            singlegroup["url"] = vArray[vArray.size() - 2];
            singlegroup["interval"] = stoi(vArray[vArray.size() - 1]);
        }
        else
            continue;

        for(unsigned int i = 2; i < rules_upper_bound; i++)
        {
            if(vArray[i].find("[]") == 0)
            {
                filtered_nodelist.emplace_back(vArray[i].substr(2));
            }
            else
            {
                for(std::string &y : nodelist)
                {
                    if(regFind(y, vArray[i]) && std::find(filtered_nodelist.begin(), filtered_nodelist.end(), y) == filtered_nodelist.end())
                        filtered_nodelist.emplace_back(y);
                }
            }
        }

        if(!filtered_nodelist.size())
            filtered_nodelist.emplace_back("DIRECT");

        singlegroup["name"] = vArray[0];
        singlegroup["type"] = vArray[1];
        singlegroup["proxies"] = vectorToNode(filtered_nodelist);

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

    yamlnode["Proxy Group"] = original_groups;

    rulesetToClash(yamlnode, ruleset_content_array);

    return to_string(yamlnode);
}
