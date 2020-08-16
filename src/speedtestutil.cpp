#include <fstream>
#include <algorithm>
#include <cmath>
#include <time.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "yamlcpp_extra.h"
#include "misc.h"
#include "printout.h"
#include "logger.h"
#include "speedtestutil.h"
#include "webget.h"
#include "rapidjson_extra.h"
#include "ini_reader.h"
#include "string_hash.h"

using namespace rapidjson;
using namespace YAML;

string_array ss_ciphers = {"rc4-md5", "aes-128-gcm", "aes-192-gcm", "aes-256-gcm", "aes-128-cfb", "aes-192-cfb", "aes-256-cfb", "aes-128-ctr", "aes-192-ctr", "aes-256-ctr", "camellia-128-cfb", "camellia-192-cfb", "camellia-256-cfb", "bf-cfb", "chacha20-ietf-poly1305", "xchacha20-ietf-poly1305", "salsa20", "chacha20", "chacha20-ietf"};
string_array ssr_ciphers = {"none", "table", "rc4", "rc4-md5", "aes-128-cfb", "aes-192-cfb", "aes-256-cfb", "aes-128-ctr", "aes-192-ctr", "aes-256-ctr", "bf-cfb", "camellia-128-cfb", "camellia-192-cfb", "camellia-256-cfb", "cast5-cfb", "des-cfb", "idea-cfb", "rc2-cfb", "seed-cfb", "salsa20", "chacha20", "chacha20-ietf"};

std::map<std::string, std::string> parsedMD5;
std::string modSSMD5 = "f7653207090ce3389115e9c88541afe0";

//remake from speedtestutil

void explodeVmess(std::string vmess, const std::string &custom_port, nodeInfo &node)
{
    std::string version, ps, add, port, type, id, aid, net, path, host, tls;
    Document jsondata;
    std::vector<std::string> vArray;
    if(regMatch(vmess, "vmess://(.*?)@(.*)"))
    {
        explodeStdVMess(vmess, custom_port, node);
        return;
    }
    else if(regMatch(vmess, "vmess://(.*?)\\?(.*)")) //shadowrocket style link
    {
        explodeShadowrocket(vmess, custom_port, node);
        return;
    }
    else if(regMatch(vmess, "vmess1://(.*?)\\?(.*)")) //kitsunebi style link
    {
        explodeKitsunebi(vmess, custom_port, node);
        return;
    }
    vmess = urlsafe_base64_decode(regReplace(vmess, "(vmess|vmess1)://", ""));
    if(regMatch(vmess, "(.*?) = (.*)"))
    {
        explodeQuan(vmess, custom_port, node);
        return;
    }
    jsondata.Parse(vmess.data());
    if(jsondata.HasParseError())
        return;

    version = "1"; //link without version will treat as version 1
    GetMember(jsondata, "v", version); //try to get version

    GetMember(jsondata, "ps", ps);
    GetMember(jsondata, "add", add);
    port = custom_port.size() ? custom_port : GetMember(jsondata, "port");
    if(port == "0")
        return;
    GetMember(jsondata, "type", type);
    GetMember(jsondata, "id", id);
    GetMember(jsondata, "aid", aid);
    GetMember(jsondata, "net", net);
    GetMember(jsondata, "tls", tls);

    GetMember(jsondata, "host", host);
    switch(to_int(version))
    {
    case 1:
        if(host.size())
        {
            vArray = split(host, ";");
            if(vArray.size() == 2)
            {
                host = vArray[0];
                path = vArray[1];
            }
        }
        break;
    case 2:
        path = GetMember(jsondata, "path");
        break;
    }

    add = trim(add);
    node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
    node.group = V2RAY_DEFAULT_GROUP;
    node.remarks = ps;
    node.server = add;
    node.port = to_int(port, 1);
    node.proxyStr = vmessConstruct(node.group, ps, add, port, type, id, aid, net, "auto", path, host, "", tls);
}

void explodeVmessConf(std::string content, const std::string &custom_port, bool libev, std::vector<nodeInfo> &nodes)
{
    nodeInfo node;
    Document json;
    rapidjson::Value nodejson, settings;
    std::string group, ps, add, port, type, id, aid, net, path, host, edge, tls, cipher, subid;
    tribool udp, tfo, scv;
    int configType, index = nodes.size();
    std::map<std::string, std::string> subdata;
    std::map<std::string, std::string>::iterator iter;
    std::string streamset = "streamSettings", tcpset = "tcpSettings", wsset = "wsSettings";
    regGetMatch(content, "((?i)streamsettings)", 2, 0, &streamset);
    regGetMatch(content, "((?i)tcpsettings)", 2, 0, &tcpset);
    regGetMatch(content, "((?1)wssettings)", 2, 0, &wsset);

    json.Parse(content.data());
    if(json.HasParseError())
        return;
    try
    {
        if(json.HasMember("outbounds")) //single config
        {
            if(json["outbounds"].Size() > 0 && json["outbounds"][0].HasMember("settings") && json["outbounds"][0]["settings"].HasMember("vnext") && json["outbounds"][0]["settings"]["vnext"].Size() > 0)
            {
                nodejson = json["outbounds"][0];
                add = GetMember(nodejson["settings"]["vnext"][0], "address");
                port = custom_port.size() ? custom_port : GetMember(nodejson["settings"]["vnext"][0], "port");
                if(port == "0")
                    return;
                if(nodejson["settings"]["vnext"][0]["users"].Size())
                {
                    id = GetMember(nodejson["settings"]["vnext"][0]["users"][0], "id");
                    aid = GetMember(nodejson["settings"]["vnext"][0]["users"][0], "alterId");
                    cipher = GetMember(nodejson["settings"]["vnext"][0]["users"][0], "security");
                }
                if(nodejson.HasMember(streamset.data()))
                {
                    net = GetMember(nodejson[streamset.data()], "network");
                    tls = GetMember(nodejson[streamset.data()], "security");
                    if(net == "ws")
                    {
                        if(nodejson[streamset.data()].HasMember(wsset.data()))
                            settings = nodejson[streamset.data()][wsset.data()];
                        else
                            settings.RemoveAllMembers();
                        path = GetMember(settings, "path");
                        if(settings.HasMember("headers"))
                        {
                            host = GetMember(settings["headers"], "Host");
                            edge = GetMember(settings["headers"], "Edge");
                        }
                    }
                    if(nodejson[streamset.data()].HasMember(tcpset.data()))
                        settings = nodejson[streamset.data()][tcpset.data()];
                    else
                        settings.RemoveAllMembers();
                    if(settings.IsObject() && settings.HasMember("header"))
                    {
                        type = GetMember(settings["header"], "type");
                        if(type == "http")
                        {
                            if(settings["header"].HasMember("request"))
                            {
                                if(settings["header"]["request"].HasMember("path") && settings["header"]["request"]["path"].Size())
                                    settings["header"]["request"]["path"][0] >> path;
                                if(settings["header"]["request"].HasMember("headers"))
                                {
                                    host = GetMember(settings["header"]["request"]["headers"], "Host");
                                    edge = GetMember(settings["header"]["request"]["headers"], "Edge");
                                }
                            }
                        }
                    }
                }
                node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
                node.group = V2RAY_DEFAULT_GROUP;
                node.remarks = add + ":" + port;
                node.server = add;
                node.port = to_int(port, 1);
                node.proxyStr = vmessConstruct(node.group, node.remarks, add, port, type, id, aid, net, cipher, path, host, edge, tls, udp, tfo, scv);
                nodes.emplace_back(std::move(node));
                node = nodeInfo();
            }
            return;
        }
    }
    catch(std::exception & e)
    {
        writeLog(0, "VMessConf parser throws an error. Leaving...", LOG_LEVEL_WARNING);
        return;
        //ignore
    }
    //read all subscribe remark as group name
    for(unsigned int i = 0; i < json["subItem"].Size(); i++)
        subdata.insert(std::pair<std::string, std::string>(json["subItem"][i]["id"].GetString(), json["subItem"][i]["remarks"].GetString()));

    for(unsigned int i = 0; i < json["vmess"].Size(); i++)
    {
        if(json["vmess"][i]["address"].IsNull() || json["vmess"][i]["port"].IsNull() || json["vmess"][i]["id"].IsNull())
            continue;

        //common info
        json["vmess"][i]["remarks"] >> ps;
        json["vmess"][i]["address"] >> add;
        port = custom_port.size() ? custom_port : GetMember(json["vmess"][i], "port");
        if(port == "0")
            continue;
        json["vmess"][i]["subid"] >> subid;

        if(subid.size())
        {
            iter = subdata.find(subid);
            if(iter != subdata.end())
                group = iter->second;
        }
        if(ps.empty())
            ps = add + ":" + port;

        scv = GetMember(json["vmess"][i], "allowInsecure");
        json["vmess"][i]["configType"] >> configType;
        switch(configType)
        {
        case 1: //vmess config
            json["vmess"][i]["headerType"] >> type;
            json["vmess"][i]["id"] >> id;
            json["vmess"][i]["alterId"] >> aid;
            json["vmess"][i]["network"] >> net;
            json["vmess"][i]["path"] >> path;
            json["vmess"][i]["requestHost"] >> host;
            json["vmess"][i]["streamSecurity"] >> tls;
            json["vmess"][i]["security"] >> cipher;
            group = V2RAY_DEFAULT_GROUP;
            node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
            node.proxyStr = vmessConstruct(group, ps, add, port, type, id, aid, net, cipher, path, host, "", tls, udp, tfo, scv);
            break;
        case 3: //ss config
            json["vmess"][i]["id"] >> id;
            json["vmess"][i]["security"] >> cipher;
            group = SS_DEFAULT_GROUP;
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
            node.proxyStr = ssConstruct(group, ps, add, port, id, cipher, "", "", libev, udp, tfo, scv);
            break;
        case 4: //socks config
            group = SOCKS_DEFAULT_GROUP;
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
            node.proxyStr = socksConstruct(group, ps, add, port, "", "", udp, tfo, scv);
            break;
        default:
            continue;
        }

        node.group = group;
        node.remarks = ps;
        node.id = index;
        node.server = add;
        node.port = to_int(port, 1);
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
    }
    return;
}

void explodeSS(std::string ss, bool libev, const std::string &custom_port, nodeInfo &node)
{
    std::string ps, password, method, server, port, plugins, plugin, pluginopts, addition, group = SS_DEFAULT_GROUP, secret;
    //std::vector<std::string> args, secret;
    ss = replace_all_distinct(ss.substr(5), "/?", "?");
    if(strFind(ss, "#"))
    {
        ps = UrlDecode(ss.substr(ss.find("#") + 1));
        ss.erase(ss.find("#"));
    }

    if(strFind(ss, "?"))
    {
        addition = ss.substr(ss.find("?") + 1);
        plugins = UrlDecode(getUrlArg(addition, "plugin"));
        plugin = plugins.substr(0, plugins.find(";"));
        pluginopts = plugins.substr(plugins.find(";") + 1);
        if(getUrlArg(addition, "group").size())
            group = urlsafe_base64_decode(getUrlArg(addition, "group"));
        ss.erase(ss.find("?"));
    }
    if(strFind(ss, "@"))
    {
        if(regGetMatch(ss, "(.*?)@(.*):(.*)", 4, 0, &secret, &server, &port))
            return;
        if(regGetMatch(urlsafe_base64_decode(secret), "(.*?):(.*)", 3, 0, &method, &password))
            return;
    }
    else
    {
        if(regGetMatch(urlsafe_base64_decode(ss), "(.*?):(.*)@(.*):(.*)", 5, 0, &method, &password, &server, &port))
            return;
    }
    if(custom_port.size())
        port = custom_port;
    if(port == "0")
        return;
    if(ps.empty())
        ps = server + ":" + port;

    node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
    node.group = group;
    node.remarks = ps;
    node.server = server;
    node.port = to_int(port, 1);
    node.proxyStr = ssConstruct(group, ps, server, port, password, method, plugin, pluginopts, libev);
}

void explodeSSD(std::string link, bool libev, const std::string &custom_port, std::vector<nodeInfo> &nodes)
{
    Document jsondata;
    nodeInfo node;
    unsigned int index = nodes.size(), listType = 0, listCount = 0;
    std::string group, port, method, password, server, remarks;
    std::string plugin, pluginopts;
    std::map<unsigned int, std::string> node_map;

    link = urlsafe_base64_decode(link.substr(6));
    jsondata.Parse(link.c_str());
    if(jsondata.HasParseError())
        return;
    if(!jsondata.HasMember("servers"))
        return;
    GetMember(jsondata, "airport", group);

    if(jsondata["servers"].IsArray())
    {
        listType = 0;
        listCount = jsondata["servers"].Size();
    }
    else if(jsondata["servers"].IsObject())
    {
        listType = 1;
        listCount = jsondata["servers"].MemberCount();
        unsigned int node_index = 0;
        for(rapidjson::Value::MemberIterator iter = jsondata["servers"].MemberBegin(); iter != jsondata["servers"].MemberEnd(); iter++)
        {
            node_map.emplace(node_index, iter->name.GetString());
            node_index++;
        }
    }
    else
        return;

    rapidjson::Value singlenode;
    for(unsigned int i = 0; i < listCount; i++)
    {
        //get default info
        GetMember(jsondata, "port", port);
        GetMember(jsondata, "encryption", method);
        GetMember(jsondata, "password", password);
        GetMember(jsondata, "plugin", plugin);
        GetMember(jsondata, "plugin_options", pluginopts);

        //get server-specific info
        switch(listType)
        {
        case 0:
            singlenode = jsondata["servers"][i];
            break;
        case 1:
            singlenode = jsondata["servers"].FindMember(node_map[i].data())->value;
            break;
        default:
            continue;
        }
        singlenode["server"] >> server;
        GetMember(singlenode, "remarks", remarks);
        GetMember(singlenode, "port", port);
        GetMember(singlenode, "encryption", method);
        GetMember(singlenode, "password", password);
        GetMember(singlenode, "plugin", plugin);
        GetMember(singlenode, "plugin_options", pluginopts);

        if(custom_port.size())
            port = custom_port;
        if(port == "0")
            continue;

        node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
        node.group = group;
        node.remarks = remarks;
        node.server = server;
        node.port = to_int(port, 1);
        node.proxyStr = ssConstruct(group, remarks, server, port, password, method, plugin, pluginopts, libev);
        node.id = index;
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
        index++;
    }
    return;
}

void explodeSSAndroid(std::string ss, bool libev, const std::string &custom_port, std::vector<nodeInfo> &nodes)
{
    std::string ps, password, method, server, port, group = SS_DEFAULT_GROUP;
    std::string plugin, pluginopts;

    Document json;
    nodeInfo node;
    int index = nodes.size();
    //first add some extra data before parsing
    ss = "{\"nodes\":" + ss + "}";
    json.Parse(ss.data());
    if(json.HasParseError())
        return;

    for(unsigned int i = 0; i < json["nodes"].Size(); i++)
    {
        server = GetMember(json["nodes"][i], "server");
        if(server.empty())
            continue;
        ps = GetMember(json["nodes"][i], "remarks");
        port = custom_port.size() ? custom_port : GetMember(json["nodes"][i], "server_port");
        if(port == "0")
            continue;
        if(ps.empty())
            ps = server + ":" + port;
        password = GetMember(json["nodes"][i], "password");
        method = GetMember(json["nodes"][i], "method");
        plugin = GetMember(json["nodes"][i], "plugin");
        pluginopts = GetMember(json["nodes"][i], "plugin_opts");

        node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
        node.id = index;
        node.group = group;
        node.remarks = ps;
        node.server = server;
        node.port = to_int(port, 1);
        node.proxyStr = ssConstruct(group, ps, server, port, password, method, plugin, pluginopts, libev);
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
        index++;
    }
}

void explodeSSConf(std::string content, const std::string &custom_port, bool libev, std::vector<nodeInfo> &nodes)
{
    nodeInfo node;
    Document json;
    std::string ps, password, method, server, port, plugin, pluginopts, group = SS_DEFAULT_GROUP;
    int index = nodes.size();

    json.Parse(content.data());
    if(json.HasParseError())
        return;
    const char *section = json.HasMember("version") && json.HasMember("remarks") && json.HasMember("servers") ? "servers" : "configs";
    if(!json.HasMember(section))
        return;
    GetMember(json, "remarks", group);

    for(unsigned int i = 0; i < json[section].Size(); i++)
    {
        ps = GetMember(json[section][i], "remarks");
        port = custom_port.size() ? custom_port : GetMember(json[section][i], "server_port");
        if(port == "0")
            continue;
        if(ps.empty())
            ps = server + ":" + port;

        password = GetMember(json[section][i], "password");
        method = GetMember(json[section][i], "method");
        server = GetMember(json[section][i], "server");
        plugin = GetMember(json[section][i], "plugin");
        pluginopts = GetMember(json[section][i], "plugin_opts");

        node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
        node.group = group;
        node.remarks = ps;
        node.id = index;
        node.server = server;
        node.port = to_int(port, 1);
        node.proxyStr = ssConstruct(group, ps, server, port, password, method, plugin, pluginopts, libev);
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
        index++;
    }
    return;
}

void explodeSSR(std::string ssr, bool ss_libev, bool ssr_libev, const std::string &custom_port, nodeInfo &node)
{
    std::string strobfs;
    std::string remarks, group, server, port, method, password, protocol, protoparam, obfs, obfsparam, remarks_base64;
    ssr = replace_all_distinct(ssr.substr(6), "\r", "");
    ssr = urlsafe_base64_decode(ssr);
    if(strFind(ssr, "/?"))
    {
        strobfs = ssr.substr(ssr.find("/?") + 2);
        ssr = ssr.substr(0, ssr.find("/?"));
        group = urlsafe_base64_decode(getUrlArg(strobfs, "group"));
        remarks = urlsafe_base64_decode(getUrlArg(strobfs, "remarks"));
        remarks_base64 = urlsafe_base64_reverse(getUrlArg(strobfs, "remarks"));
        obfsparam = regReplace(urlsafe_base64_decode(getUrlArg(strobfs, "obfsparam")), "\\s", "");
        protoparam = regReplace(urlsafe_base64_decode(getUrlArg(strobfs, "protoparam")), "\\s", "");
    }

    if(regGetMatch(ssr, "(.*):(.*?):(.*?):(.*?):(.*?):(.*)", 7, 0, &server, &port, &protocol, &method, &obfs, &password))
        return;
    password = urlsafe_base64_decode(password);
    if(custom_port.size())
        port = custom_port;
    if(port == "0")
        return;

    if(group.empty())
        group = SSR_DEFAULT_GROUP;
    if(remarks.empty())
    {
        remarks = server + ":" + port;
        remarks_base64 = base64_encode(remarks);
    }

    node.group = group;
    node.remarks = remarks;
    node.server = server;
    node.port = to_int(port, 1);
    if(find(ss_ciphers.begin(), ss_ciphers.end(), method) != ss_ciphers.end() && (obfs.empty() || obfs == "plain") && (protocol.empty() || protocol == "origin"))
    {
        node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
        node.proxyStr = ssConstruct(group, remarks, server, port, password, method, "", "", ss_libev);
    }
    else
    {
        node.linkType = SPEEDTEST_MESSAGE_FOUNDSSR;
        node.proxyStr = ssrConstruct(group, remarks, remarks_base64, server, port, protocol, method, obfs, password, obfsparam, protoparam, ssr_libev);
    }
}

void explodeSSRConf(std::string content, const std::string &custom_port, bool ss_libev, bool ssr_libev, std::vector<nodeInfo> &nodes)
{
    nodeInfo node;
    Document json;
    std::string remarks, remarks_base64, group, server, port, method, password, protocol, protoparam, obfs, obfsparam, plugin, pluginopts;
    int index = nodes.size();

    json.Parse(content.data());
    if(json.HasParseError())
        return;

    if(json.HasMember("local_port") && json.HasMember("local_address")) //single libev config
    {
        server = GetMember(json, "server");
        port = GetMember(json, "server_port");
        node.remarks = server + ":" + port;
        node.server = server;
        node.port = to_int(port, 1);
        method = GetMember(json, "method");
        obfs = GetMember(json, "obfs");
        protocol = GetMember(json, "protocol");
        if(find(ss_ciphers.begin(), ss_ciphers.end(), method) != ss_ciphers.end() && (obfs.empty() || obfs == "plain") && (protocol.empty() || protocol == "origin"))
        {
            plugin = GetMember(json, "plugin");
            pluginopts = GetMember(json, "plugin_opts");
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
            node.group = SS_DEFAULT_GROUP;
            node.proxyStr = ssConstruct(node.group, node.remarks, server, port, password, method, plugin, pluginopts, ss_libev);
        }
        else
        {
            protoparam = GetMember(json, "protocol_param");
            obfsparam = GetMember(json, "obfs_param");
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSSR;
            node.group = SSR_DEFAULT_GROUP;
            node.proxyStr = ssrConstruct(node.group, node.remarks, base64_encode(node.remarks), server, port, protocol, method, obfs, password, obfsparam, protoparam, ssr_libev);
        }
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
        return;
    }

    for(unsigned int i = 0; i < json["configs"].Size(); i++)
    {
        group = GetMember(json["configs"][i], "group");
        if(group.empty())
            group = SSR_DEFAULT_GROUP;
        remarks = GetMember(json["configs"][i], "remarks");
        server = GetMember(json["configs"][i], "server");
        port = custom_port.size() ? custom_port : GetMember(json["configs"][i], "server_port");
        if(port == "0")
            continue;
        if(remarks.empty())
            remarks = server + ":" + port;

        remarks_base64 = GetMember(json["configs"][i], "remarks_base64"); // electron-ssr does not contain this field
        password = GetMember(json["configs"][i], "password");
        method = GetMember(json["configs"][i], "method");

        protocol = GetMember(json["configs"][i], "protocol");
        protoparam = GetMember(json["configs"][i], "protocolparam");
        obfs = GetMember(json["configs"][i], "obfs");
        obfsparam = GetMember(json["configs"][i], "obfsparam");

        node.linkType = SPEEDTEST_MESSAGE_FOUNDSSR;
        node.group = group;
        node.remarks = remarks;
        node.id = index;
        node.server = server;
        node.port = to_int(port, 1);
        node.proxyStr = ssrConstruct(group, remarks, remarks_base64, server, port, protocol, method, obfs, password, obfsparam, protoparam, ssr_libev);
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
        index++;
    }
    return;
}

void explodeSocks(std::string link, const std::string &custom_port, nodeInfo &node)
{
    std::string group, remarks, server, port, username, password;
    if(strFind(link, "socks://")) //v2rayn socks link
    {
        std::vector<std::string> arguments;
        if(strFind(link, "#"))
        {
            remarks = UrlDecode(link.substr(link.find("#") + 1));
            link.erase(link.find("#"));
        }
        link = urlsafe_base64_decode(link.substr(8));
        arguments = split(link, ":");
        if(arguments.size() < 2)
            return;
        server = arguments[0];
        port = arguments[1];
    }
    else if(strFind(link, "https://t.me/socks") || strFind(link, "tg://socks")) //telegram style socks link
    {
        server = getUrlArg(link, "server");
        port = getUrlArg(link, "port");
        username = UrlDecode(getUrlArg(link, "user"));
        password = UrlDecode(getUrlArg(link, "pass"));
        remarks = UrlDecode(getUrlArg(link, "remarks"));
        group = UrlDecode(getUrlArg(link, "group"));
    }
    if(group.empty())
        group = SOCKS_DEFAULT_GROUP;
    if(remarks.empty())
        remarks = server + ":" + port;
    if(custom_port.size())
        port = custom_port;
    if(port == "0")
        return;

    node.linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
    node.group = group;
    node.remarks = remarks;
    node.server = server;
    node.port = to_int(port, 1);
    node.proxyStr = socksConstruct(group, remarks, server, port, username, password);
}

void explodeHTTP(const std::string &link, const std::string &custom_port, nodeInfo &node)
{
    std::string group, remarks, server, port, username, password;
    server = getUrlArg(link, "server");
    port = getUrlArg(link, "port");
    username = UrlDecode(getUrlArg(link, "user"));
    password = UrlDecode(getUrlArg(link, "pass"));
    remarks = UrlDecode(getUrlArg(link, "remarks"));
    group = UrlDecode(getUrlArg(link, "group"));

    if(group.empty())
        group = HTTP_DEFAULT_GROUP;
    if(remarks.empty())
        remarks = server + ":" + port;
    if(custom_port.size())
        port = custom_port;
    if(port == "0")
        return;

    node.linkType = SPEEDTEST_MESSAGE_FOUNDHTTP;
    node.group = group;
    node.remarks = remarks;
    node.server = server;
    node.port = to_int(port, 1);
    node.proxyStr = httpConstruct(group, remarks, server, port, username, password, strFind(link, "/https"));
}

void explodeHTTPSub(std::string link, const std::string &custom_port, nodeInfo &node)
{
    std::string group, remarks, server, port, username, password;
    std::string addition;
    bool tls = strFind(link, "https://");
    string_size pos = link.find("?");
    if(pos != link.npos)
    {
        addition = link.substr(pos + 1);
        link.erase(pos);
        remarks = UrlDecode(getUrlArg(addition, "remarks"));
        group = UrlDecode(getUrlArg(addition, "group"));
    }
    link.erase(0, link.find("://") + 3);
    link = urlsafe_base64_decode(link);
    if(strFind(link, "@"))
    {
        if(regGetMatch(link, "(.*?):(.*?)@(.*):(.*)", 5, 0, &username, &password, &server, &port))
            return;
    }
    else
    {
        if(regGetMatch(link, "(.*):(.*)", 3, 0, &server, &port))
            return;
    }

    if(group.empty())
        group = HTTP_DEFAULT_GROUP;
    if(remarks.empty())
        remarks = server + ":" + port;
    if(custom_port.size())
        port = custom_port;
    if(port == "0")
        return;

    node.linkType = SPEEDTEST_MESSAGE_FOUNDHTTP;
    node.group = group;
    node.remarks = remarks;
    node.server = server;
    node.port = to_int(port, 1);
    node.proxyStr = httpConstruct(group, remarks, server, port, username, password, tls);
}

void explodeTrojan(std::string trojan, const std::string &custom_port, nodeInfo &node)
{
    std::string server, port, psk, addition, group, remark, host;
    tribool tfo, scv;
    trojan.erase(0, 9);
    string_size pos = trojan.rfind("#");

    if(pos != trojan.npos)
    {
        remark = UrlDecode(trojan.substr(pos + 1));
        trojan.erase(pos);
    }
    pos = trojan.find("?");
    if(pos != trojan.npos)
    {
        addition = trojan.substr(pos + 1);
        trojan.erase(pos);
    }

    if(regGetMatch(trojan, "(.*?)@(.*):(.*)", 4, 0, &psk, &server, &port))
        return;
    if(custom_port.size())
        port = custom_port;
    if(port == "0")
        return;

    host = getUrlArg(addition, "peer");
    tfo = getUrlArg(addition, "tfo");
    scv = getUrlArg(addition, "allowInsecure");
    group = UrlDecode(getUrlArg(addition, "group"));

    if(remark.empty())
        remark = server + ":" + port;
    if(host.empty() && !isIPv4(server) && !isIPv6(server))
        host = server;
    if(group.empty())
        group = TROJAN_DEFAULT_GROUP;

    node.linkType = SPEEDTEST_MESSAGE_FOUNDTROJAN;
    node.group = group;
    node.remarks = remark;
    node.server = server;
    node.port = to_int(port, 1);
    node.proxyStr = trojanConstruct(group, remark, server, port, psk, host, true, tribool(), tfo, scv);
}

void explodeQuan(const std::string &quan, const std::string &custom_port, nodeInfo &node)
{
    std::string strTemp, itemName, itemVal;
    std::string group = V2RAY_DEFAULT_GROUP, ps, add, port, cipher, type = "none", id, aid = "0", net = "tcp", path, host, edge, tls;
    string_array configs, vArray, headers;
    strTemp = regReplace(quan, "(.*?) = (.*)", "$1,$2");
    configs = split(strTemp, ",");

    if(configs[1] == "vmess")
    {
        if(configs.size() < 6)
            return;
        ps = trim(configs[0]);
        add = trim(configs[2]);
        port = custom_port.size() ? custom_port : trim(configs[3]);
        if(port == "0")
            return;
        cipher = trim(configs[4]);
        id = trim(replace_all_distinct(configs[5], "\"", ""));

        //read link
        for(unsigned int i = 6; i < configs.size(); i++)
        {
            vArray = split(configs[i], "=");
            if(vArray.size() < 2)
                continue;
            itemName = trim(vArray[0]);
            itemVal = trim(vArray[1]);
            switch(hash_(itemName))
            {
                case "group"_hash: group = itemVal; break;
                case "over-tls"_hash: tls = itemVal == "true" ? "tls" : ""; break;
                case "tls-host"_hash: host = itemVal; break;
                case "obfs-path"_hash: path = replace_all_distinct(itemVal, "\"", ""); break;
                case "obfs-header"_hash:
                    headers = split(replace_all_distinct(replace_all_distinct(itemVal, "\"", ""), "[Rr][Nn]", "|"), "|");
                    for(std::string &x : headers)
                    {
                        if(regFind(x, "(?i)Host: "))
                            host = x.substr(6);
                        else if(regFind(x, "(?i)Edge: "))
                            edge = x.substr(6);
                    }
                    break;
                case "obfs"_hash:
                    if(itemVal == "ws")
                        net = "ws";
                    break;
                default: continue;
            }
        }
        if(path.empty())
            path = "/";

        node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
        node.group = group;
        node.remarks = ps;
        node.server = add;
        node.port = to_int(port, 1);
        node.proxyStr = vmessConstruct(group, ps, add, port, type, id, aid, net, cipher, path, host, edge, tls);
    }
}

void explodeNetch(std::string netch, bool ss_libev, bool ssr_libev, const std::string &custom_port, nodeInfo &node)
{
    Document json;
    std::string type, group, remark, address, port, username, password, method, plugin, pluginopts, protocol, protoparam, obfs, obfsparam, id, aid, transprot, faketype, host, edge, path, tls;
    tribool udp, tfo, scv;
    netch = urlsafe_base64_decode(netch.substr(8));

    json.Parse(netch.data());
    if(json.HasParseError())
        return;
    type = GetMember(json, "Type");
    group = GetMember(json, "Group");
    remark = GetMember(json, "Remark");
    address = GetMember(json, "Hostname");
    udp = GetMember(json, "EnableUDP");
    tfo = GetMember(json, "EnableTFO");
    scv = GetMember(json, "AllowInsecure");
    port = custom_port.size() ? custom_port : GetMember(json, "Port");
    if(port == "0")
        return;
    method = GetMember(json, "EncryptMethod");
    password = GetMember(json, "Password");
    if(remark.empty())
        remark = address + ":" + port;
    switch(hash_(type))
    {
    case "SS"_hash:
        plugin = GetMember(json, "Plugin");
        pluginopts = GetMember(json, "PluginOption");
        if(group.empty())
            group = SS_DEFAULT_GROUP;
        node.group = group;
        node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
        node.proxyStr = ssConstruct(group, remark, address, port, password, method, plugin, pluginopts, ss_libev, udp, tfo, scv);
        break;
    case "SSR"_hash:
        protocol = GetMember(json, "Protocol");
        obfs = GetMember(json, "OBFS");
        if(find(ss_ciphers.begin(), ss_ciphers.end(), method) != ss_ciphers.end() && (obfs.empty() || obfs == "plain") && (protocol.empty() || protocol == "origin"))
        {
            plugin = GetMember(json, "Plugin");
            pluginopts = GetMember(json, "PluginOption");
            if(group.empty())
                group = SS_DEFAULT_GROUP;
            node.group = group;
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
            node.proxyStr = ssConstruct(group, remark, address, port, password, method, plugin, pluginopts, ss_libev, udp, tfo, scv);
        }
        else
        {
            protoparam = GetMember(json, "ProtocolParam");
            obfsparam = GetMember(json, "OBFSParam");
            if(group.empty())
                group = SSR_DEFAULT_GROUP;
            node.group = group;
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSSR;
            node.proxyStr = ssrConstruct(group, remark, base64_encode(remark), address, port, protocol, method, obfs, password, obfsparam, protoparam, ssr_libev, udp, tfo, scv);
        }
        break;
    case "VMess"_hash:
        id = GetMember(json, "UserID");
        aid = GetMember(json, "AlterID");
        transprot = GetMember(json, "TransferProtocol");
        faketype = GetMember(json, "FakeType");
        host = GetMember(json, "Host");
        path = GetMember(json, "Path");
        edge = GetMember(json, "Edge");
        tls = GetMember(json, "TLSSecure");
        node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
        if(group.empty())
            group = V2RAY_DEFAULT_GROUP;
        node.group = group;
        node.proxyStr = vmessConstruct(group, remark, address, port, faketype, id, aid, transprot, method, path, host, edge, tls, udp, tfo, scv);
        break;
    case "Socks5"_hash:
        username = GetMember(json, "Username");
        node.linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
        if(group.empty())
            group = SOCKS_DEFAULT_GROUP;
        node.group = group;
        node.proxyStr = socksConstruct(group, remark, address, port, username, password, udp, tfo, scv);
        break;
    case "HTTP"_hash:
    case "HTTPS"_hash:
        node.linkType = SPEEDTEST_MESSAGE_FOUNDHTTP;
        if(group.empty())
            group = HTTP_DEFAULT_GROUP;
        node.group = group;
        node.proxyStr = httpConstruct(group, remark, address, port, username, password, type == "HTTPS", tfo, scv);
        break;
    case "Trojan"_hash:
        host = GetMember(json, "Host");
        tls = GetMember(json, "TLSSecure");
        node.linkType = SPEEDTEST_MESSAGE_FOUNDTROJAN;
        if(group.empty())
            group = TROJAN_DEFAULT_GROUP;
        node.group = group;
        node.proxyStr = trojanConstruct(group, remark, address, port, password, host, tls == "true", udp, tfo, scv);
        break;
    case "Snell"_hash:
        obfs = GetMember(json, "OBFS");
        host = GetMember(json, "Host");
        node.linkType = SPEEDTEST_MESSAGE_FOUNDSNELL;
        if(group.empty())
            group = SNELL_DEFAULT_GROUP;
        node.group = group;
        node.proxyStr = snellConstruct(group, remark, address, port, password, obfs, host, udp, tfo, scv);
        break;
    default:
        return;
    }

    node.remarks = remark;
    node.server = address;
    node.port = (unsigned short)to_int(port, 1);
}

void explodeClash(Node yamlnode, const std::string &custom_port, std::vector<nodeInfo> &nodes, bool ss_libev, bool ssr_libev)
{
    std::string proxytype, ps, server, port, cipher, group, password; //common
    std::string type = "none", id, aid = "0", net = "tcp", path, host, edge, tls; //vmess
    std::string plugin, pluginopts, pluginopts_mode, pluginopts_host, pluginopts_mux; //ss
    std::string protocol, protoparam, obfs, obfsparam; //ssr
    std::string user; //socks
    tribool udp, tfo, scv;
    nodeInfo node;
    Node singleproxy;
    unsigned int index = nodes.size();
    const std::string section = yamlnode["proxies"].IsDefined() ? "proxies" : "Proxy";
    for(unsigned int i = 0; i < yamlnode[section].size(); i++)
    {
        singleproxy = yamlnode[section][i];
        singleproxy["type"] >>= proxytype;
        singleproxy["name"] >>= ps;
        singleproxy["server"] >>= server;
        port = custom_port.empty() ? safe_as<std::string>(singleproxy["port"]) : custom_port;
        if(port.empty() || port == "0")
            continue;
        udp = safe_as<std::string>(singleproxy["udp"]);
        scv = safe_as<std::string>(singleproxy["skip-cert-verify"]);
        switch(hash_(proxytype))
        {
        case "vmess"_hash:
            group = V2RAY_DEFAULT_GROUP;

            singleproxy["uuid"] >>= id;
            singleproxy["alterId"] >>= aid;
            singleproxy["cipher"] >>= cipher;
            net = singleproxy["network"].IsDefined() ? safe_as<std::string>(singleproxy["network"]) : "tcp";
            if(net == "http")
            {
                singleproxy["http-opts"]["path"][0] >>= path;
                singleproxy["http-opts"]["headers"]["Host"][0] >>= host;
                edge.clear();
            }
            else
            {
                path = singleproxy["ws-path"].IsDefined() ? safe_as<std::string>(singleproxy["ws-path"]) : "/";
                singleproxy["ws-headers"]["Host"] >>= host;
                singleproxy["ws-headers"]["Edge"] >>= edge;
            }
            tls = safe_as<std::string>(singleproxy["tls"]) == "true" ? "tls" : "";

            node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
            node.proxyStr = vmessConstruct(group, ps, server, port, "", id, aid, net, cipher, path, host, edge, tls, udp, tfo, scv);
            break;
        case "ss"_hash:
            group = SS_DEFAULT_GROUP;

            singleproxy["cipher"] >>= cipher;
            singleproxy["password"] >>= password;
            if(singleproxy["plugin"].IsDefined())
            {
                switch(hash_(safe_as<std::string>(singleproxy["plugin"])))
                {
                case "obfs"_hash:
                    plugin = "simple-obfs";
                    if(singleproxy["plugin-opts"].IsDefined())
                    {
                        singleproxy["plugin-opts"]["mode"] >>= pluginopts_mode;
                        singleproxy["plugin-opts"]["host"] >>= pluginopts_host;
                    }
                    break;
                case "v2ray-plugin"_hash:
                    plugin = "v2ray-plugin";
                    if(singleproxy["plugin-opts"].IsDefined())
                    {
                        singleproxy["plugin-opts"]["mode"] >>= pluginopts_mode;
                        singleproxy["plugin-opts"]["host"] >>= pluginopts_host;
                        tls = safe_as<bool>(singleproxy["plugin-opts"]["tls"]) ? "tls;" : "";
                        singleproxy["plugin-opts"]["path"] >>= path;
                        pluginopts_mux = safe_as<bool>(singleproxy["plugin-opts"]["mux"]) ? "mux=4;" : "";
                    }
                    break;
                default:
                    break;
                }
            }
            else if(singleproxy["obfs"].IsDefined())
            {
                plugin = "simple-obfs";
                singleproxy["obfs"] >>= pluginopts_mode;
                singleproxy["obfs-host"] >>= pluginopts_host;
            }
            else
                plugin.clear();

            switch(hash_(plugin))
            {
            case "simple-obfs"_hash:
            case "obfs-local"_hash:
                pluginopts = "obfs=" + pluginopts_mode;
                pluginopts += pluginopts_host.empty() ? "" : ";obfs-host=" + pluginopts_host;
                break;
            case "v2ray-plugin"_hash:
                pluginopts = "mode=" + pluginopts_mode + ";" + tls + pluginopts_mux;
                if(pluginopts_host.size())
                    pluginopts += "host=" + pluginopts_host + ";";
                if(path.size())
                    pluginopts += "path=" + path + ";";
                if(pluginopts_mux.size())
                    pluginopts += "mux=" + pluginopts_mux + ";";
                break;
            }

            //support for go-shadowsocks2
            if(cipher == "AEAD_CHACHA20_POLY1305")
                cipher = "chacha20-ietf-poly1305";
            else if(strFind(cipher, "AEAD"))
            {
                cipher = replace_all_distinct(replace_all_distinct(cipher, "AEAD_", ""), "_", "-");
                std::transform(cipher.begin(), cipher.end(), cipher.begin(), ::tolower);
            }

            node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
            node.proxyStr = ssConstruct(group, ps, server, port, password, cipher, plugin, pluginopts, ss_libev, udp, tfo, scv);
            break;
        case "socks"_hash:
            group = SOCKS_DEFAULT_GROUP;

            singleproxy["username"] >>= user;
            singleproxy["password"] >>= password;

            node.linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
            node.proxyStr = socksConstruct(group, ps, server, port, user, password);
            break;
        case "ssr"_hash:
            group = SSR_DEFAULT_GROUP;

            singleproxy["cipher"] >>= cipher;
            singleproxy["password"] >>= password;
            singleproxy["protocol"] >>= protocol;
            singleproxy["obfs"] >>= obfs;
            if(singleproxy["protocol-param"].IsDefined())
                singleproxy["protocol-param"] >>= protoparam;
            else
                singleproxy["protocolparam"] >>= protoparam;
            if(singleproxy["obfs-param"].IsDefined())
                singleproxy["obfs-param"] >>= obfsparam;
            else
                singleproxy["obfsparam"] >>= obfsparam;

            node.linkType = SPEEDTEST_MESSAGE_FOUNDSSR;
            node.proxyStr = ssrConstruct(group, ps, base64_encode(ps), server, port, protocol, cipher, obfs, password, obfsparam, protoparam, ssr_libev, udp, tfo, scv);
            break;
        case "http"_hash:
            group = HTTP_DEFAULT_GROUP;

            singleproxy["username"] >>= user;
            singleproxy["password"] >>= password;
            singleproxy["tls"] >>= tls;

            node.linkType = SPEEDTEST_MESSAGE_FOUNDHTTP;
            node.proxyStr = httpConstruct(group, ps, server, port, user, password, tls == "true", tfo, scv);
            break;
        case "trojan"_hash:
            group = TROJAN_DEFAULT_GROUP;
            singleproxy["password"] >>= password;
            singleproxy["sni"] >>= host;

            node.linkType = SPEEDTEST_MESSAGE_FOUNDTROJAN;
            node.proxyStr = trojanConstruct(group, ps, server, port, password, host, true, udp, tfo, scv);
            break;
        case "snell"_hash:
            group = SNELL_DEFAULT_GROUP;
            singleproxy["psk"] >> password;
            singleproxy["obfs-opts"]["mode"] >>= obfs;
            singleproxy["obfs-opts"]["host"] >>= host;

            node.linkType = SPEEDTEST_MESSAGE_FOUNDSNELL;
            node.proxyStr = snellConstruct(group, ps, server, port, password, obfs, host, udp, tfo, scv);
            break;
        default:
            continue;
        }

        node.group = group;
        node.remarks = ps;
        node.server = server;
        node.port = to_int(port, 1);
        node.id = index;
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
        index++;
    }
    return;
}

void explodeStdVMess(std::string vmess, const std::string &custom_port, nodeInfo &node)
{
    std::string add, port, type, id, aid, net, path, host, tls, remarks;
    std::string addition;
    vmess = vmess.substr(8);
    string_size pos;

    pos = vmess.rfind("#");
    if(pos != vmess.npos)
    {
        remarks = UrlDecode(vmess.substr(pos + 1));
        vmess.erase(pos);
    }
    const std::string stdvmess_matcher = R"(^([a-z]+)(?:\+([a-z]+))?:([\da-f]{4}(?:[\da-f]{4}-){4}[\da-f]{12})-(\d+)@(.+):(\d+)(?:\/?\?(.*))?$)";
    if(regGetMatch(vmess, stdvmess_matcher, 8, 0, &net, &tls, &id, &aid, &add, &port, &addition))
        return;

    switch(hash_(net))
    {
    case "tcp"_hash:
    case "kcp"_hash:
        type = getUrlArg(addition, "type");
        break;
    case "http"_hash:
    case "ws"_hash:
        host = getUrlArg(addition, "host");
        path = getUrlArg(addition, "path");
        break;
    case "quic"_hash:
        type = getUrlArg(addition, "security");
        host = getUrlArg(addition, "type");
        path = getUrlArg(addition, "key");
        break;
    default:
        return;
    }

    if(!custom_port.empty())
        port = custom_port;
    if(remarks.empty())
        remarks = add + ":" + port;

    node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
    node.group = V2RAY_DEFAULT_GROUP;
    node.remarks = remarks;
    node.server = add;
    node.port = to_int(port, 0);
    node.proxyStr = vmessConstruct(node.group, remarks, add, port, type, id, aid, net, "auto", path, host, "", tls);
    return;
}

void explodeShadowrocket(std::string rocket, const std::string &custom_port, nodeInfo &node)
{
    std::string add, port, type, id, aid, net = "tcp", path, host, tls, cipher, remarks;
    std::string obfs; //for other style of link
    std::string addition;
    rocket = rocket.substr(8);

    string_size pos = rocket.find("?");
    addition = rocket.substr(pos + 1);
    rocket.erase(pos);

    if(regGetMatch(urlsafe_base64_decode(rocket), "(.*?):(.*)@(.*):(.*)", 5, 0, &cipher, &id, &add, &port))
        return;
    if(custom_port.size())
        port = custom_port;
    if(port == "0")
        return;
    remarks = UrlDecode(getUrlArg(addition, "remarks"));
    obfs = getUrlArg(addition, "obfs");
    if(obfs.size())
    {
        if(obfs == "websocket")
        {
            net = "ws";
            host = getUrlArg(addition, "obfsParam");
            path = getUrlArg(addition, "path");
        }
    }
    else
    {
        net = getUrlArg(addition, "network");
        host = getUrlArg(addition, "wsHost");
        path = getUrlArg(addition, "wspath");
    }
    tls = getUrlArg(addition, "tls") == "1" ? "tls" : "";
    aid = getUrlArg(addition, "aid");

    if(aid.empty())
        aid = "0";

    if(remarks.empty())
        remarks = add + ":" + port;

    node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
    node.group = V2RAY_DEFAULT_GROUP;
    node.remarks = remarks;
    node.server = add;
    node.port = to_int(port, 0);
    node.proxyStr = vmessConstruct(node.group, remarks, add, port, type, id, aid, net, cipher, path, host, "", tls);
}

void explodeKitsunebi(std::string kit, const std::string &custom_port, nodeInfo &node)
{
    std::string add, port, type, id, aid = "0", net = "tcp", path, host, tls, cipher = "auto", remarks;
    std::string addition;
    string_size pos;
    kit = kit.substr(9);

    pos = kit.find("#");
    if(pos != kit.npos)
    {
        remarks = kit.substr(pos + 1);
        kit = kit.substr(0, pos);
    }

    pos = kit.find("?");
    addition = kit.substr(pos + 1);
    kit = kit.substr(0, pos);

    if(regGetMatch(kit, "(.*?)@(.*):(.*)", 4, 0, &id, &add, &port))
        return;
    pos = port.find("/");
    if(pos != port.npos)
    {
        path = port.substr(pos);
        port.erase(pos);
    }
    if(custom_port.size())
        port = custom_port;
    if(port == "0")
        return;
    net = getUrlArg(addition, "network");
    tls = getUrlArg(addition, "tls") == "true" ? "tls" : "";
    host = getUrlArg(addition, "ws.host");

    if(remarks.empty())
        remarks = add + ":" + port;

    node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
    node.group = V2RAY_DEFAULT_GROUP;
    node.remarks = remarks;
    node.server = add;
    node.port = to_int(port, 0);
    node.proxyStr = vmessConstruct(node.group, remarks, add, port, type, id, aid, net, cipher, path, host, "", tls);
}

bool explodeSurge(std::string surge, const std::string &custom_port, std::vector<nodeInfo> &nodes, bool libev)
{
    std::multimap<std::string, std::string> proxies;
    nodeInfo node;
    unsigned int i, index = nodes.size();
    INIReader ini;

    /*
    if(!strFind(surge, "[Proxy]"))
        return false;
    */

    ini.store_isolated_line = true;
    ini.keep_empty_section = false;
    ini.allow_dup_section_titles = true;
    ini.SetIsolatedItemsSection("Proxy");
    ini.IncludeSection("Proxy");
    ini.AddDirectSaveSection("Proxy");
    if(surge.find("[Proxy]") != surge.npos)
        surge = regReplace(surge, "^[\\S\\s]*?\\[", "[", false);
    ini.Parse(surge);

    if(!ini.SectionExist("Proxy"))
        return false;
    ini.EnterSection("Proxy");
    ini.GetItems(proxies);

    const std::string proxystr = "(.*?)\\s*=\\s*(.*)";

    for(auto &x : proxies)
    {
        std::string remarks, server, port, method, username, password; //common
        std::string plugin, pluginopts, pluginopts_mode, pluginopts_host, mod_url, mod_md5; //ss
        std::string id, net, tls, host, edge, path; //v2
        std::string protocol, protoparam; //ssr
        std::string itemName, itemVal, config;
        std::vector<std::string> configs, vArray, headers, header;
        tribool udp, tfo, scv, tls13;

        /*
        remarks = regReplace(x.second, proxystr, "$1");
        configs = split(regReplace(x.second, proxystr, "$2"), ",");
        */
        regGetMatch(x.second, proxystr, 3, 0, &remarks, &config);
        configs = split(config, ",");
        if(configs.size() < 3)
            continue;
        switch(hash_(configs[0]))
        {
        case "direct"_hash:
        case "reject"_hash:
        case "reject-tinygif"_hash:
            continue;
        case "custom"_hash: //surge 2 style custom proxy
            //remove module detection to speed up parsing and compatible with broken module
            /*
            mod_url = trim(configs[5]);
            if(parsedMD5.count(mod_url) > 0)
            {
                mod_md5 = parsedMD5[mod_url]; //read calculated MD5 from map
            }
            else
            {
                mod_md5 = getMD5(webGet(mod_url)); //retrieve module and calculate MD5
                parsedMD5.insert(std::pair<std::string, std::string>(mod_url, mod_md5)); //save unrecognized module MD5 to map
            }
            */

            //if(mod_md5 == modSSMD5) //is SSEncrypt module
            {
                if(configs.size() < 5)
                    continue;
                server = trim(configs[1]);
                port = custom_port.empty() ? trim(configs[2]) : custom_port;
                if(port == "0")
                    continue;
                method = trim(configs[3]);
                password = trim(configs[4]);

                for(i = 6; i < configs.size(); i++)
                {
                    vArray = split(configs[i], "=");
                    if(vArray.size() < 2)
                        continue;
                    itemName = trim(vArray[0]);
                    itemVal = trim(vArray[1]);
                    switch(hash_(itemName))
                    {
                        case "obfs"_hash:
                            plugin = "simple-obfs";
                            pluginopts_mode = itemVal;
                            break;
                        case "obfs-host"_hash: pluginopts_host = itemVal; break;
                        case "udp-relay"_hash: udp = itemVal; break;
                        case "tfo"_hash: tfo = itemVal; break;
                        default: continue;
                    }
                }
                if(plugin.size())
                {
                    pluginopts = "obfs=" + pluginopts_mode;
                    pluginopts += pluginopts_host.empty() ? "" : ";obfs-host=" + pluginopts_host;
                }

                node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
                node.group = SS_DEFAULT_GROUP;
                node.proxyStr = ssConstruct(node.group, remarks, server, port, password, method, plugin, pluginopts, libev, udp, tfo, scv);
            }
            //else
            //    continue;
            break;
        case "ss"_hash: //surge 3 style ss proxy
            server = trim(configs[1]);
            port = custom_port.empty() ? trim(configs[2]) : custom_port;
            if(port == "0")
                continue;

            for(i = 3; i < configs.size(); i++)
            {
                vArray = split(configs[i], "=");
                if(vArray.size() < 2)
                    continue;
                itemName = trim(vArray[0]);
                itemVal = trim(vArray[1]);
                switch(hash_(itemName))
                {
                    case "encrypt-method"_hash: method = itemVal; break;
                    case "password"_hash: password = itemVal; break;
                    case "obfs"_hash:
                        plugin = "simple-obfs";
                        pluginopts_mode = itemVal;
                        break;
                    case "obfs-host"_hash: pluginopts_host = itemVal; break;
                    case "udp-relay"_hash: udp = itemVal; break;
                    case "tfo"_hash: tfo = itemVal; break;
                    default: continue;
                }
            }
            if(plugin.size())
            {
                pluginopts = "obfs=" + pluginopts_mode;
                pluginopts += pluginopts_host.empty() ? "" : ";obfs-host=" + pluginopts_host;
            }

            node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
            node.group = SS_DEFAULT_GROUP;
            node.proxyStr = ssConstruct(node.group, remarks, server, port, password, method, plugin, pluginopts, libev, udp, tfo, scv);
            break;
        case "socks5"_hash: //surge 3 style socks5 proxy
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
            node.group = SOCKS_DEFAULT_GROUP;
            server = trim(configs[1]);
            port = custom_port.empty() ? trim(configs[2]) : custom_port;
            if(port == "0")
                continue;
            if(configs.size() >= 5)
            {
                username = trim(configs[3]);
                password = trim(configs[4]);
            }
            for(i = 5; i < configs.size(); i++)
            {
                vArray = split(configs[i], "=");
                if(vArray.size() < 2)
                    continue;
                itemName = trim(vArray[0]);
                itemVal = trim(vArray[1]);
                switch(hash_(itemName))
                {
                    case "udp-relay"_hash: udp = itemVal; break;
                    case "tfo"_hash: tfo = itemVal; break;
                    case "skip-cert-verify"_hash: scv = itemVal; break;
                    default: continue;
                }
            }
            node.proxyStr = socksConstruct(node.group, remarks, server, port, username, password, udp, tfo, scv);
            break;
        case "vmess"_hash: //surge 4 style vmess proxy
            server = trim(configs[1]);
            port = custom_port.empty() ? trim(configs[2]) : custom_port;
            if(port == "0")
                continue;
            net = "tcp";
            method = "auto";

            for(i = 3; i < configs.size(); i++)
            {
                vArray = split(configs[i], "=");
                if(vArray.size() != 2)
                    continue;
                itemName = trim(vArray[0]);
                itemVal = trim(vArray[1]);
                switch(hash_(itemName))
                {
                    case "username"_hash: id = itemVal; break;
                    case "ws"_hash: net = itemVal == "true" ? "ws" : "tcp"; break;
                    case "tls"_hash: tls = itemVal == "true" ? "tls" : ""; break;
                    case "ws-path"_hash: path = itemVal; break;
                    case "obfs-host"_hash: host = itemVal; break;
                    case "ws-headers"_hash:
                        headers = split(itemVal, "|");
                        for(auto &y : headers)
                        {
                            header = split(trim(y), ":");
                            if(header.size() != 2)
                                continue;
                            else if(regMatch(header[0], "(?i)host"))
                                host = trim_quote(header[1]);
                            else if(regMatch(header[0], "(?i)edge"))
                                edge = trim_quote(header[1]);
                        }
                        break;
                    case "udp-relay"_hash: udp = itemVal; break;
                    case "tfo"_hash: tfo = itemVal; break;
                    case "skip-cert-verify"_hash: scv = itemVal; break;
                    case "tls13"_hash: tls13 = itemVal; break;
                    default: continue;
                }
            }
            if(host.empty() && !isIPv4(server) && !isIPv6(server))
                host = server;

            node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
            node.group = V2RAY_DEFAULT_GROUP;
            node.proxyStr = vmessConstruct(node.group, remarks, server, port, "", id, "0", net, method, path, host, edge, tls, udp, tfo, scv, tls13);
            break;
        case "http"_hash: //http proxy
            node.linkType = SPEEDTEST_MESSAGE_FOUNDHTTP;
            node.group = HTTP_DEFAULT_GROUP;
            server = trim(configs[1]);
            port = custom_port.empty() ? trim(configs[2]) : custom_port;
            if(port == "0")
                continue;
            for(i = 3; i < configs.size(); i++)
            {
                vArray = split(configs[i], "=");
                if(vArray.size() < 2)
                    continue;
                itemName = trim(vArray[0]);
                itemVal = trim(vArray[1]);
                switch(hash_(itemName))
                {
                    case "username"_hash: username = itemVal; break;
                    case "password"_hash: password = itemVal; break;
                    case "skip-cert-verify"_hash: scv = itemVal; break;
                    default: continue;
                }
            }
            node.proxyStr = httpConstruct(node.group, remarks, server, port, username, password, false, tfo, scv);
            break;
        case "trojan"_hash: // surge 4 style trojan proxy
            node.linkType = SPEEDTEST_MESSAGE_FOUNDTROJAN;
            node.group = TROJAN_DEFAULT_GROUP;
            server = trim(configs[1]);
            port = custom_port.empty() ? trim(configs[2]) : custom_port;
            if(port == "0")
                continue;

            for(i = 3; i < configs.size(); i++)
            {
                vArray = split(configs[i], "=");
                if(vArray.size() != 2)
                    continue;
                itemName = trim(vArray[0]);
                itemVal = trim(vArray[1]);
                switch(hash_(itemName))
                {
                    case "password"_hash: password = itemVal; break;
                    case "sni"_hash: host = itemVal; break;
                    case "udp-relay"_hash: udp = itemVal; break;
                    case "tfo"_hash: tfo = itemVal; break;
                    case "skip-cert-verify"_hash: scv = itemVal; break;
                    default: continue;
                }
            }
            if(host.empty() && !isIPv4(server) && !isIPv6(server))
                host = server;

            node.proxyStr = trojanConstruct(node.group, remarks, server, port, password, host, true, udp, tfo, scv);
            break;
        case "snell"_hash:
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSNELL;
            node.group = SNELL_DEFAULT_GROUP;

            server = trim(configs[1]);
            port = custom_port.empty() ? trim(configs[2]) : custom_port;
            if(port == "0")
                continue;

            for(i = 3; i < configs.size(); i++)
            {
                vArray = split(configs[i], "=");
                if(vArray.size() != 2)
                    continue;
                itemName = trim(vArray[0]);
                itemVal = trim(vArray[1]);
                switch(hash_(itemName))
                {
                    case "psk"_hash: password = itemVal; break;
                    case "obfs"_hash: plugin = itemVal; break;
                    case "obfs-host"_hash: host = itemVal; break;
                    case "udp-relay"_hash: udp = itemVal; break;
                    case "tfo"_hash: tfo = itemVal; break;
                    case "skip-cert-verify"_hash: scv = itemVal; break;
                    default: continue;
                }
            }
            if(host.empty() && !isIPv4(server) && !isIPv6(server))
                host = server;

            node.proxyStr = snellConstruct(node.group, remarks, server, port, password, plugin, host, udp, tfo, scv);
            break;
        default:
            switch(hash_(remarks))
            {
            case "shadowsocks"_hash: //quantumult x style ss/ssr link
                server = trim(configs[0].substr(0, configs[0].rfind(":")));
                port = custom_port.empty() ? trim(configs[0].substr(configs[0].rfind(":") + 1)) : custom_port;
                if(port == "0")
                    continue;

                for(i = 1; i < configs.size(); i++)
                {
                    vArray = split(trim(configs[i]), "=");
                    if(vArray.size() != 2)
                        continue;
                    itemName = trim(vArray[0]);
                    itemVal = trim(vArray[1]);
                    switch(hash_(itemName))
                    {
                        case "method"_hash: method = itemVal; break;
                        case "password"_hash: password = itemVal; break;
                        case "tag"_hash: remarks = itemVal; break;
                        case "ssr-protocol"_hash: protocol = itemVal; break;
                        case "ssr-protocol-param"_hash: protoparam = itemVal; break;
                        case "obfs"_hash:
                        {
                            switch(hash_(itemVal))
                            {
                            case "http"_hash:
                            case "tls"_hash:
                                plugin = "simple-obfs";
                                pluginopts_mode = itemVal;
                                break;
                            case "wss"_hash:
                                tls = "tls";
                                [[fallthrough]];
                            case "ws"_hash:
                                pluginopts_mode = "websocket";
                                plugin = "v2ray-plugin";
                                break;
                            default:
                                pluginopts_mode = itemVal;
                            }
                            break;
                        }
                        case "obfs-host"_hash: pluginopts_host = itemVal; break;
                        case "obfs-uri"_hash: path = itemVal; break;
                        case "udp-relay"_hash: udp = itemVal; break;
                        case "fast-open"_hash: tfo = itemVal; break;
                        case "tls13"_hash: tls13 = itemVal; break;
                        default: continue;
                    }
                }
                if(remarks.empty())
                    remarks = server + ":" + port;
                switch(hash_(plugin))
                {
                case "simple-obfs"_hash:
                    pluginopts = "obfs=" + pluginopts_mode;
                    if(pluginopts_host.size())
                        pluginopts += ";obfs-host=" + pluginopts_host;
                    break;
                case "v2ray-plugin"_hash:
                    if(pluginopts_host.empty() && !isIPv4(server) && !isIPv6(server))
                        pluginopts_host = server;
                    pluginopts = "mode=" + pluginopts_mode;
                    if(pluginopts_host.size())
                        pluginopts += ";host=" + pluginopts_host;
                    if(path.size())
                        pluginopts += ";path=" + path;
                    pluginopts += ";" + tls;
                    break;
                }

                if(protocol.size())
                {
                    node.linkType = SPEEDTEST_MESSAGE_FOUNDSSR;
                    node.group = SSR_DEFAULT_GROUP;
                    node.proxyStr = ssrConstruct(node.group, remarks, base64_encode(remarks), server, port, protocol, method, pluginopts_mode, password, pluginopts_host, protoparam, libev, udp, tfo, scv);
                }
                else
                {
                    node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
                    node.group = SS_DEFAULT_GROUP;
                    node.proxyStr = ssConstruct(node.group, remarks, server, port, password, method, plugin, pluginopts, libev, udp, tfo, scv, tls13);
                }
                break;
            case "vmess"_hash: //quantumult x style vmess link
                server = trim(configs[0].substr(0, configs[0].rfind(":")));
                port = custom_port.empty() ? trim(configs[0].substr(configs[0].rfind(":") + 1)) : custom_port;
                if(port == "0")
                    continue;
                net = "tcp";

                for(i = 1; i < configs.size(); i++)
                {
                    vArray = split(trim(configs[i]), "=");
                    if(vArray.size() != 2)
                        continue;
                    itemName = trim(vArray[0]);
                    itemVal = trim(vArray[1]);
                    switch(hash_(itemName))
                    {
                        case "method"_hash: method = itemVal; break;
                        case "password"_hash: id = itemVal; break;
                        case "tag"_hash: remarks = itemVal; break;
                        case "obfs"_hash:
                            switch(hash_(itemVal))
                            {
                                case "ws"_hash: net = "ws"; break;
                                case "over-tls"_hash: tls = "tls"; break;
                                case "wss"_hash: net = "ws"; tls = "tls"; break;
                            }
                            break;
                        case "obfs-host"_hash: host = itemVal; break;
                        case "obfs-uri"_hash: path = itemVal; break;
                        case "over-tls"_hash: tls = itemVal == "true" ? "tls" : ""; break;
                        case "udp-relay"_hash: udp = itemVal; break;
                        case "fast-open"_hash: tfo = itemVal; break;
                        case "tls13"_hash: tls13 = itemVal; break;
                        default: continue;
                    }
                }
                if(remarks.empty())
                    remarks = server + ":" + port;

                if(host.empty() && !isIPv4(server) && !isIPv6(server))
                    host = server;

                node.linkType = SPEEDTEST_MESSAGE_FOUNDVMESS;
                node.group = V2RAY_DEFAULT_GROUP;
                node.proxyStr = vmessConstruct(node.group, remarks, server, port, "", id, "0", net, method, path, host, "", tls, udp, tfo, scv, tls13);
                break;
            case "trojan"_hash: //quantumult x style trojan link
                server = trim(configs[0].substr(0, configs[0].rfind(":")));
                port = custom_port.empty() ? trim(configs[0].substr(configs[0].rfind(":") + 1)) : custom_port;
                if(port == "0")
                    continue;

                for(i = 1; i < configs.size(); i++)
                {
                    vArray = split(trim(configs[i]), "=");
                    if(vArray.size() != 2)
                        continue;
                    itemName = trim(vArray[0]);
                    itemVal = trim(vArray[1]);
                    switch(hash_(itemName))
                    {
                        case "password"_hash: password = itemVal; break;
                        case "tag"_hash: remarks = itemVal; break;
                        case "over-tls"_hash: tls = itemVal; break;
                        case "tls-host"_hash: host = itemVal; break;
                        case "udp-relay"_hash: udp = itemVal; break;
                        case "fast-open"_hash: tfo = itemVal; break;
                        case "tls-verification"_hash: scv = itemVal == "false"; break;
                        case "tls13"_hash: tls13 = itemVal; break;
                        default: continue;
                    }
                }
                if(remarks.empty())
                    remarks = server + ":" + port;

                if(host.empty() && !isIPv4(server) && !isIPv6(server))
                    host = server;

                node.linkType = SPEEDTEST_MESSAGE_FOUNDTROJAN;
                node.group = TROJAN_DEFAULT_GROUP;
                node.proxyStr = trojanConstruct(node.group, remarks, server, port, password, host, tls == "true", udp, tfo, scv, tls13);
                break;
            case "http"_hash: //quantumult x style http links
                server = trim(configs[0].substr(0, configs[0].rfind(":")));
                port = custom_port.empty() ? trim(configs[0].substr(configs[0].rfind(":") + 1)) : custom_port;
                if(port == "0")
                    continue;

                for(i = 1; i < configs.size(); i++)
                {
                    vArray = split(trim(configs[i]), "=");
                    if(vArray.size() != 2)
                        continue;
                    itemName = trim(vArray[0]);
                    itemVal = trim(vArray[1]);
                    switch(hash_(itemName))
                    {
                        case "username"_hash: username = itemVal; break;
                        case "password"_hash: password = itemVal; break;
                        case "tag"_hash: remarks = itemVal; break;
                        case "over-tls"_hash: tls = itemVal; break;
                        case "tls-verification"_hash: scv = itemVal == "false"; break;
                        case "tls13"_hash: tls13 = itemVal; break;
                        case "fast-open"_hash: tfo = itemVal; break;
                        default: continue;
                    }
                }
                if(remarks.empty())
                    remarks = server + ":" + port;

                if(host.empty() && !isIPv4(server) && !isIPv6(server))
                    host = server;

                if(username == "none")
                    username.clear();
                if(password == "none")
                    password.clear();

                node.linkType = SPEEDTEST_MESSAGE_FOUNDHTTP;
                node.group = HTTP_DEFAULT_GROUP;
                node.proxyStr = httpConstruct(node.group, remarks, server, port, username, password, tls == "true", tfo, scv, tls13);
                break;
            default:
                continue;
            }
            break;
        }

        node.remarks = remarks;
        node.server = server;
        node.port = to_int(port);
        node.id = index;
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
        index++;
    }
    return index;
}

void explodeSSTap(std::string sstap, const std::string &custom_port, std::vector<nodeInfo> &nodes, bool ss_libev, bool ssr_libev)
{
    std::string configType, group, remarks, server, port;
    std::string cipher;
    std::string user, pass;
    std::string protocol, protoparam, obfs, obfsparam;
    Document json;
    nodeInfo node;
    unsigned int index = nodes.size();
    json.Parse(sstap.data());
    if(json.HasParseError())
        return;

    for(unsigned int i = 0; i < json["configs"].Size(); i++)
    {
        json["configs"][i]["group"] >> group;
        json["configs"][i]["remarks"] >> remarks;
        json["configs"][i]["server"] >> server;
        port = custom_port.size() ? custom_port : GetMember(json["configs"][i], "server_port");
        if(port == "0")
            continue;

        if(remarks.empty())
            remarks = server + ":" + port;

        json["configs"][i]["password"] >> pass;
        json["configs"][i]["type"] >> configType;
        switch(to_int(configType, 0))
        {
        case 5: //socks 5
            json["configs"][i]["username"] >> user;
            node.linkType = SPEEDTEST_MESSAGE_FOUNDSOCKS;
            node.proxyStr = socksConstruct(group, remarks, server, port, user, pass);
            break;
        case 6: //ss/ssr
            json["configs"][i]["protocol"] >> protocol;
            json["configs"][i]["obfs"] >> obfs;
            json["configs"][i]["method"] >> cipher;
            if(find(ss_ciphers.begin(), ss_ciphers.end(), cipher) != ss_ciphers.end() && protocol == "origin" && obfs == "plain") //is ss
            {
                node.linkType = SPEEDTEST_MESSAGE_FOUNDSS;
                node.proxyStr = ssConstruct(group, remarks, server, port, pass, cipher, "", "", ss_libev);
            }
            else //is ssr cipher
            {
                json["configs"][i]["obfsparam"] >> obfsparam;
                json["configs"][i]["protocolparam"] >> protoparam;
                node.linkType = SPEEDTEST_MESSAGE_FOUNDSSR;
                node.proxyStr = ssrConstruct(group, remarks, base64_encode(remarks), server, port, protocol, cipher, obfs, pass, obfsparam, protoparam, ssr_libev);
            }
            break;
        default:
            continue;
        }

        node.group = group;
        node.remarks = remarks;
        node.id = index;
        node.server = server;
        node.port = to_int(port, 1);
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
    }
}

void explodeNetchConf(std::string netch, bool ss_libev, bool ssr_libev, const std::string &custom_port, std::vector<nodeInfo> &nodes)
{
    Document json;
    nodeInfo node;
    unsigned int index = nodes.size();

    json.Parse(netch.data());
    if(json.HasParseError())
        return;

    if(!json.HasMember("Server"))
        return;

    for(unsigned int i = 0; i < json["Server"].Size(); i++)
    {
        explodeNetch("Netch://" + base64_encode(SerializeObject(json["Server"][i])), ss_libev, ssr_libev, custom_port, node);

        node.id = index;
        nodes.emplace_back(std::move(node));
        node = nodeInfo();
        index++;
    }
}

bool chkIgnore(const nodeInfo &node, string_array &exclude_remarks, string_array &include_remarks)
{
    bool excluded = false, included = false;
    //std::string remarks = UTF8ToACP(node.remarks);
    std::string remarks = node.remarks;
    //writeLog(LOG_TYPE_INFO, "Comparing exclude remarks...");
    excluded = std::any_of(exclude_remarks.cbegin(), exclude_remarks.cend(), [&remarks](const auto &x)
    {
        return regFind(remarks, x);
    });
    if(include_remarks.size() != 0)
    {
        //writeLog(LOG_TYPE_INFO, "Comparing include remarks...");
        included = std::any_of(include_remarks.cbegin(), include_remarks.cend(), [&remarks](const auto &x)
        {
            return regFind(remarks, x);
        });
    }
    else
    {
        included = true;
    }

    return excluded || !included;
}

int explodeConf(std::string filepath, const std::string &custom_port, bool sslibev, bool ssrlibev, std::vector<nodeInfo> &nodes)
{
    std::ifstream infile;
    std::stringstream contentstrm;
    infile.open(filepath);

    contentstrm << infile.rdbuf();
    infile.close();

    return explodeConfContent(contentstrm.str(), custom_port, sslibev, ssrlibev, nodes);
}

int explodeConfContent(const std::string &content, const std::string &custom_port, bool sslibev, bool ssrlibev, std::vector<nodeInfo> &nodes)
{
    int filetype = -1;

    if(strFind(content, "\"version\""))
        filetype = SPEEDTEST_MESSAGE_FOUNDSS;
    else if(strFind(content, "\"serverSubscribes\""))
        filetype = SPEEDTEST_MESSAGE_FOUNDSSR;
    else if(strFind(content, "\"uiItem\"") || strFind(content, "vnext"))
        filetype = SPEEDTEST_MESSAGE_FOUNDVMESS;
    else if(strFind(content, "\"proxy_apps\""))
        filetype = SPEEDTEST_MESSAGE_FOUNDSSCONF;
    else if(strFind(content, "\"idInUse\""))
        filetype = SPEEDTEST_MESSAGE_FOUNDSSTAP;
    else if(strFind(content, "\"local_address\"") && strFind(content, "\"local_port\""))
        filetype = SPEEDTEST_MESSAGE_FOUNDSSR; //use ssr config parser
    else if(strFind(content, "\"ModeFileNameType\""))
        filetype = SPEEDTEST_MESSAGE_FOUNDNETCH;

    switch(filetype)
    {
    case SPEEDTEST_MESSAGE_FOUNDSS:
        explodeSSConf(content, custom_port, sslibev, nodes);
        break;
    case SPEEDTEST_MESSAGE_FOUNDSSR:
        explodeSSRConf(content, custom_port, sslibev, ssrlibev, nodes);
        break;
    case SPEEDTEST_MESSAGE_FOUNDVMESS:
        explodeVmessConf(content, custom_port, sslibev, nodes);
        break;
    case SPEEDTEST_MESSAGE_FOUNDSSCONF:
        explodeSSAndroid(content, sslibev, custom_port, nodes);
        break;
    case SPEEDTEST_MESSAGE_FOUNDSSTAP:
        explodeSSTap(content, custom_port, nodes, sslibev, ssrlibev);
        break;
    case SPEEDTEST_MESSAGE_FOUNDNETCH:
        explodeNetchConf(content, sslibev, ssrlibev, custom_port, nodes);
        break;
    default:
        //try to parse as a local subscription
        explodeSub(content, sslibev, ssrlibev, custom_port, nodes);
    }

    if(nodes.size() == 0)
        return SPEEDTEST_ERROR_UNRECOGFILE;
    else
        return SPEEDTEST_ERROR_NONE;
}

void explode(const std::string &link, bool sslibev, bool ssrlibev, const std::string &custom_port, nodeInfo &node)
{
    // TODO: replace strFind with startsWith if appropriate
    if(strFind(link, "ssr://"))
        explodeSSR(link, sslibev, ssrlibev, custom_port, node);
    else if(strFind(link, "vmess://") || strFind(link, "vmess1://"))
        explodeVmess(link, custom_port, node);
    else if(strFind(link, "ss://"))
        explodeSS(link, sslibev, custom_port, node);
    else if(strFind(link, "socks://") || strFind(link, "https://t.me/socks") || strFind(link, "tg://socks"))
        explodeSocks(link, custom_port, node);
    else if(strFind(link, "https://t.me/http") || strFind(link, "tg://http")) //telegram style http link
        explodeHTTP(link, custom_port, node);
    else if(strFind(link, "Netch://"))
        explodeNetch(link, sslibev, ssrlibev, custom_port, node);
    else if(strFind(link, "trojan://"))
        explodeTrojan(link, custom_port, node);
    else if(isLink(link))
        explodeHTTPSub(link, custom_port, node);
}

void explodeSub(std::string sub, bool sslibev, bool ssrlibev, const std::string &custom_port, std::vector<nodeInfo> &nodes)
{
    std::stringstream strstream;
    std::string strLink;
    bool processed = false;
    nodeInfo node;

    //try to parse as SSD configuration
    if(startsWith(sub, "ssd://"))
    {
        explodeSSD(sub, sslibev, custom_port, nodes);
        processed = true;
    }

    //try to parse as clash configuration
    try
    {
        if(!processed && regFind(sub, "\"?(Proxy|proxies)\"?:"))
        {
            regGetMatch(sub, R"(^(?:Proxy|proxies):$\s(?:(?:^ +?.*$| *?-.*$|)\s?)+)", 1, &sub);
            Node yamlnode = Load(sub);
            if(yamlnode.size() && (yamlnode["Proxy"].IsDefined() || yamlnode["proxies"].IsDefined()))
            {
                explodeClash(yamlnode, custom_port, nodes, sslibev, ssrlibev);
                processed = true;
            }
        }
    }
    catch (std::exception &e)
    {
        //ignore
    }

    //try to parse as surge configuration
    if(!processed && explodeSurge(sub, custom_port, nodes, sslibev))
    {
        processed = true;
    }

    //try to parse as normal subscription
    if(!processed)
    {
        sub = urlsafe_base64_decode(sub);
        if(regFind(sub, "(vmess|shadowsocks|http|trojan)\\s*?="))
        {
            if(explodeSurge(sub, custom_port, nodes, sslibev))
                return;
        }
        strstream << sub;
        char delimiter = count(sub.begin(), sub.end(), '\n') < 1 ? count(sub.begin(), sub.end(), '\r') < 1 ? ' ' : '\r' : '\n';
        while(getline(strstream, strLink, delimiter))
        {
            if(strLink.rfind("\r") != strLink.npos)
                strLink.erase(strLink.size() - 1);
            node.linkType = -1;
            explode(strLink, sslibev, ssrlibev, custom_port, node);
            if(strLink.size() == 0 || node.linkType == -1)
            {
                continue;
            }
            nodes.emplace_back(std::move(node));
            node = nodeInfo();
        }
    }
}

void filterNodes(std::vector<nodeInfo> &nodes, string_array &exclude_remarks, string_array &include_remarks, int groupID)
{
    int node_index = 0;
    std::vector<nodeInfo>::iterator iter = nodes.begin();
    /*
    while(iter != nodes.end())
    {
        if(chkIgnore(*iter, exclude_remarks, include_remarks))
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
                case PCRE2_ERROR_NOMATCH: break;
                default: return;
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
                    case PCRE2_ERROR_NOMATCH: break;
                    default: return;
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
    writeLog(LOG_TYPE_INFO, "Filter done.");
}

unsigned long long streamToInt(const std::string &stream)
{
    if(!stream.size())
        return 0;
    double streamval = 1.0;
    std::vector<std::string> units = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    size_t index = units.size();
    do
    {
        index--;
        if(endsWith(stream, units[index]))
        {
            streamval = std::pow(1024, index) * to_number<float>(stream.substr(0, stream.size() - units[index].size()), 0.0);
            break;
        }
    } while(index != 0);
    return (unsigned long long)streamval;
}

static inline double percentToDouble(const std::string &percent)
{
    return stof(percent.substr(0, percent.size() - 1)) / 100.0;
}

time_t dateStringToTimestamp(std::string date)
{
    time_t rawtime;
    time(&rawtime);
    if(startsWith(date, "left="))
    {
        time_t seconds_left = 0;
        date.erase(0, 5);
        if(endsWith(date, "d"))
        {
            date.erase(date.size() - 1);
            seconds_left = to_number<double>(date, 0.0) * 86400.0;
        }
        return rawtime + seconds_left;
    }
    else
    {
        struct tm *expire_time;
        std::vector<std::string> date_array = split(date, ":");
        if(date_array.size() != 6)
            return 0;

        expire_time = localtime(&rawtime);
        expire_time->tm_year = to_int(date_array[0], 1900) - 1900;
        expire_time->tm_mon = to_int(date_array[1], 1) - 1;
        expire_time->tm_mday = to_int(date_array[2]);
        expire_time->tm_hour = to_int(date_array[3]);
        expire_time->tm_min = to_int(date_array[4]);
        expire_time->tm_sec = to_int(date_array[5]);
        return mktime(expire_time);
    }
}

bool getSubInfoFromHeader(const std::string &header, std::string &result)
{
    std::string pattern = R"(^(?i:Subscription-UserInfo): (.*?)\s*?$)", retStr;
    if(regFind(header, pattern))
    {
        regGetMatch(header, pattern, 2, 0, &retStr);
        if(retStr.size())
        {
            result = retStr;
            return true;
        }
    }
    return false;
}

bool getSubInfoFromNodes(const std::vector<nodeInfo> &nodes, const string_array &stream_rules, const string_array &time_rules, std::string &result)
{
    std::string remarks, pattern, target, stream_info, time_info, retStr;
    string_size spos;

    for(const nodeInfo &x : nodes)
    {
        remarks = x.remarks;
        if(!stream_info.size())
        {
            for(const std::string &y : stream_rules)
            {
                spos = y.rfind("|");
                if(spos == y.npos)
                    continue;
                pattern = y.substr(0, spos);
                target = y.substr(spos + 1);
                if(regMatch(remarks, pattern))
                {
                    retStr = regReplace(remarks, pattern, target);
                    if(retStr != remarks)
                    {
                        stream_info = retStr;
                        break;
                    }
                }
                else
                    continue;
            }
        }

        remarks = x.remarks;
        if(!time_info.size())
        {
            for(const std::string &y : time_rules)
            {
                spos = y.rfind("|");
                if(spos == y.npos)
                    continue;
                pattern = y.substr(0, spos);
                target = y.substr(spos + 1);
                if(regMatch(remarks, pattern))
                {
                    retStr = regReplace(remarks, pattern, target);
                    if(retStr != remarks)
                    {
                        time_info = retStr;
                        break;
                    }
                }
                else
                    continue;
            }
        }

        if(stream_info.size() && time_info.size())
            break;
    }

    if(!stream_info.size() && !time_info.size())
        return false;

    //calculate how much stream left
    unsigned long long total = 0, left, used = 0, expire = 0;
    std::string total_str = getUrlArg(stream_info, "total"), left_str = getUrlArg(stream_info, "left"), used_str = getUrlArg(stream_info, "used");
    if(strFind(total_str, "%"))
    {
        if(used_str.size())
        {
            used = streamToInt(used_str);
            total = used / (1 - percentToDouble(total_str));
        }
        else if(left_str.size())
        {
            left = streamToInt(left_str);
            total = left / percentToDouble(total_str);
            used = total - left;
        }
    }
    else
    {
        total = streamToInt(total_str);
        if(used_str.size())
        {
            used = streamToInt(used_str);
        }
        else if(left_str.size())
        {
            left = streamToInt(left_str);
            used = total - left;
        }
    }

    result = "upload=0; download=" + std::to_string(used) + "; total=" + std::to_string(total) + ";";

    //calculate expire time
    expire = dateStringToTimestamp(time_info);
    if(expire)
        result += " expire=" + std::to_string(expire) + ";";

    return true;
}

bool getSubInfoFromSSD(const std::string &sub, std::string &result)
{
    rapidjson::Document json;
    json.Parse(urlsafe_base64_decode(sub.substr(6)).data());
    if(json.HasParseError())
        return false;

    std::string used_str = GetMember(json, "traffic_used"), total_str = GetMember(json, "traffic_total"), expire_str = GetMember(json, "expiry");
    if(!used_str.size() || !total_str.size())
        return false;
    unsigned long long used = stod(used_str) * std::pow(1024, 3), total = stod(total_str) * std::pow(1024, 3), expire;
    result = "upload=0; download=" + std::to_string(used) + "; total=" + std::to_string(total) + ";";

    expire = dateStringToTimestamp(regReplace(expire_str, "(\\d+)-(\\d+)-(\\d+) (.*)", "$1:$2:$3:$4"));
    if(expire)
        result += " expire=" + std::to_string(expire) + ";";

    return true;
}
