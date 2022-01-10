#include <string>
#include <map>

#include "../utils/base64/base64.h"
#include "../utils/ini_reader/ini_reader.h"
#include "../utils/network.h"
#include "../utils/rapidjson_extra.h"
#include "../utils/regexp.h"
#include "../utils/string.h"
#include "../utils/string_hash.h"
#include "../utils/urlencode.h"
#include "../utils/yamlcpp_extra.h"
#include "config/proxy.h"
#include "subparser.h"

using namespace rapidjson;
using namespace YAML;

string_array ss_ciphers = {"rc4-md5", "aes-128-gcm", "aes-192-gcm", "aes-256-gcm", "aes-128-cfb", "aes-192-cfb", "aes-256-cfb", "aes-128-ctr", "aes-192-ctr", "aes-256-ctr", "camellia-128-cfb", "camellia-192-cfb", "camellia-256-cfb", "bf-cfb", "chacha20-ietf-poly1305", "xchacha20-ietf-poly1305", "salsa20", "chacha20", "chacha20-ietf"};
string_array ssr_ciphers = {"none", "table", "rc4", "rc4-md5", "aes-128-cfb", "aes-192-cfb", "aes-256-cfb", "aes-128-ctr", "aes-192-ctr", "aes-256-ctr", "bf-cfb", "camellia-128-cfb", "camellia-192-cfb", "camellia-256-cfb", "cast5-cfb", "des-cfb", "idea-cfb", "rc2-cfb", "seed-cfb", "salsa20", "chacha20", "chacha20-ietf"};

std::map<std::string, std::string> parsedMD5;
std::string modSSMD5 = "f7653207090ce3389115e9c88541afe0";

//remake from speedtestutil

void commonConstruct(Proxy &node, ProxyType type, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const tribool &udp, const tribool &tfo, const tribool &scv, const tribool &tls13)
{
    node.Type = type;
    node.Group = group;
    node.Remark = remarks;
    node.Hostname = server;
    node.Port = to_int(port);
    node.UDP = udp;
    node.TCPFastOpen = tfo;
    node.AllowInsecure = scv;
    node.TLS13 = tls13;
}

void vmessConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &add, const std::string &port, const std::string &type, const std::string &id, const std::string &aid, const std::string &net, const std::string &cipher, const std::string &path, const std::string &host, const std::string &edge, const std::string &tls, tribool udp, tribool tfo, tribool scv, tribool tls13)
{
    commonConstruct(node, ProxyType::VMess, group, remarks, add, port, udp, tfo, scv, tls13);
    node.UserId = id.empty() ? "00000000-0000-0000-0000-000000000000" : id;
    node.AlterId = to_int(aid);
    node.EncryptMethod = cipher;
    node.TransferProtocol = net.empty() ? "tcp" : net;
    node.Edge = edge;

    if(net == "quic")
    {
        node.QUICSecure = host;
        node.QUICSecret = path;
    }
    else
    {
        node.Host = host.empty() ? add.data() : trim(host);
        node.Path = path.empty() ? "/" : trim(path);
    }
    node.FakeType = type;
    node.TLSSecure = tls == "tls";
}

void ssrConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &protocol, const std::string &method, const std::string &obfs, const std::string &password, const std::string &obfsparam, const std::string &protoparam, tribool udp, tribool tfo, tribool scv)
{
    commonConstruct(node, ProxyType::ShadowsocksR, group, remarks, server, port, udp, tfo, scv, tribool());
    node.Password = password;
    node.EncryptMethod = method;
    node.Protocol = protocol;
    node.ProtocolParam = protoparam;
    node.OBFS = obfs;
    node.OBFSParam = obfsparam;
}

void ssConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &method, const std::string &plugin, const std::string &pluginopts, tribool udp, tribool tfo, tribool scv, tribool tls13)
{
    commonConstruct(node, ProxyType::Shadowsocks, group, remarks, server, port, udp, tfo, scv, tls13);
    node.Password = password;
    node.EncryptMethod = method;
    node.Plugin = plugin;
    node.PluginOption = pluginopts;
}

void socksConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &username, const std::string &password, tribool udp, tribool tfo, tribool scv)
{
    commonConstruct(node, ProxyType::SOCKS5, group, remarks, server, port, udp, tfo, scv, tribool());
    node.Username = username;
    node.Password = password;
}

void httpConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &username, const std::string &password, bool tls, tribool tfo, tribool scv, tribool tls13)
{
    commonConstruct(node, tls ? ProxyType::HTTPS : ProxyType::HTTP, group, remarks, server, port, tribool(), tfo, scv, tls13);
    node.Username = username;
    node.Password = password;
    node.TLSSecure = tls;
}

void trojanConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &network, const std::string &host, const std::string &path, bool tlssecure, tribool udp, tribool tfo, tribool scv, tribool tls13)
{
    commonConstruct(node, ProxyType::Trojan, group, remarks, server, port, udp, tfo, scv, tls13);
    node.Password = password;
    node.Host = host;
    node.TLSSecure = tlssecure;
    node.TransferProtocol = network.empty() ? "tcp" : network;
    node.Path = path;
}

void snellConstruct(Proxy &node, const std::string &group, const std::string &remarks, const std::string &server, const std::string &port, const std::string &password, const std::string &obfs, const std::string &host, tribool udp, tribool tfo, tribool scv)
{
    commonConstruct(node, ProxyType::Snell, group, remarks, server, port, udp, tfo, scv, tribool());
    node.Password = password;
    node.OBFS = obfs;
    node.Host = host;
}

void explodeVmess(std::string vmess, Proxy &node)
{
    std::string version, ps, add, port, type, id, aid, net, path, host, tls;
    Document jsondata;
    std::vector<std::string> vArray;
    if(regMatch(vmess, "vmess://(.*?)@(.*)"))
    {
        explodeStdVMess(vmess, node);
        return;
    }
    else if(regMatch(vmess, "vmess://(.*?)\\?(.*)")) //shadowrocket style link
    {
        explodeShadowrocket(vmess, node);
        return;
    }
    else if(regMatch(vmess, "vmess1://(.*?)\\?(.*)")) //kitsunebi style link
    {
        explodeKitsunebi(vmess, node);
        return;
    }
    vmess = urlSafeBase64Decode(regReplace(vmess, "(vmess|vmess1)://", ""));
    if(regMatch(vmess, "(.*?) = (.*)"))
    {
        explodeQuan(vmess, node);
        return;
    }
    jsondata.Parse(vmess.data());
    if(jsondata.HasParseError())
        return;

    version = "1"; //link without version will treat as version 1
    GetMember(jsondata, "v", version); //try to get version

    GetMember(jsondata, "ps", ps);
    GetMember(jsondata, "add", add);
    port = GetMember(jsondata, "port");
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
        if(!host.empty())
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

    vmessConstruct(node, V2RAY_DEFAULT_GROUP, ps, add, port, type, id, aid, net, "auto", path, host, "", tls);
}

void explodeVmessConf(std::string content, std::vector<Proxy> &nodes)
{
    Proxy node;
    Document json;
    rapidjson::Value nodejson, settings;
    std::string group, ps, add, port, type, id, aid, net, path, host, edge, tls, cipher, subid;
    tribool udp, tfo, scv;
    int configType;
    uint32_t index = nodes.size();
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
                port = GetMember(nodejson["settings"]["vnext"][0], "port");
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
                vmessConstruct(node, V2RAY_DEFAULT_GROUP, add + ":" + port, add, port, type, id, aid, net, cipher, path, host, edge, tls, udp, tfo, scv);
                nodes.emplace_back(std::move(node));
            }
            return;
        }
    }
    catch(std::exception & e)
    {
        //writeLog(0, "VMessConf parser throws an error. Leaving...", LOG_LEVEL_WARNING);
        //return;
        //ignore
        throw;
    }
    //read all subscribe remark as group name
    for(uint32_t i = 0; i < json["subItem"].Size(); i++)
        subdata.insert(std::pair<std::string, std::string>(json["subItem"][i]["id"].GetString(), json["subItem"][i]["remarks"].GetString()));

    for(uint32_t i = 0; i < json["vmess"].Size(); i++)
    {
        if(json["vmess"][i]["address"].IsNull() || json["vmess"][i]["port"].IsNull() || json["vmess"][i]["id"].IsNull())
            continue;

        //common info
        json["vmess"][i]["remarks"] >> ps;
        json["vmess"][i]["address"] >> add;
        port = GetMember(json["vmess"][i], "port");
        if(port == "0")
            continue;
        json["vmess"][i]["subid"] >> subid;

        if(!subid.empty())
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
            vmessConstruct(node, V2RAY_DEFAULT_GROUP, ps, add, port, type, id, aid, net, cipher, path, host, "", tls, udp, tfo, scv);
            break;
        case 3: //ss config
            json["vmess"][i]["id"] >> id;
            json["vmess"][i]["security"] >> cipher;
            ssConstruct(node, SS_DEFAULT_GROUP, ps, add, port, id, cipher, "", "", udp, tfo, scv);
            break;
        case 4: //socks config
            socksConstruct(node, SOCKS_DEFAULT_GROUP, ps, add, port, "", "", udp, tfo, scv);
            break;
        default:
            continue;
        }
        node.Id = index;
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
    return;
}

void explodeSS(std::string ss, Proxy &node)
{
    std::string ps, password, method, server, port, plugins, plugin, pluginopts, addition, group = SS_DEFAULT_GROUP, secret;
    //std::vector<std::string> args, secret;
    ss = replaceAllDistinct(ss.substr(5), "/?", "?");
    if(strFind(ss, "#"))
    {
        ps = urlDecode(ss.substr(ss.find("#") + 1));
        ss.erase(ss.find("#"));
    }

    if(strFind(ss, "?"))
    {
        addition = ss.substr(ss.find("?") + 1);
        plugins = urlDecode(getUrlArg(addition, "plugin"));
        plugin = plugins.substr(0, plugins.find(";"));
        pluginopts = plugins.substr(plugins.find(";") + 1);
        group = getUrlArg(addition, "group");
        if(!group.empty())
            group = urlSafeBase64Decode(group);
        ss.erase(ss.find("?"));
    }
    if(strFind(ss, "@"))
    {
        if(regGetMatch(ss, "(\\S+?)@(\\S+):(\\d+)", 4, 0, &secret, &server, &port))
            return;
        if(regGetMatch(urlSafeBase64Decode(secret), "(\\S+?):(\\S+)", 3, 0, &method, &password))
            return;
    }
    else
    {
        if(regGetMatch(urlSafeBase64Decode(ss), "(\\S+?):(\\S+)@(\\S+):(\\d+)", 5, 0, &method, &password, &server, &port))
            return;
    }
    if(port == "0")
        return;
    if(ps.empty())
        ps = server + ":" + port;

    ssConstruct(node, group, ps, server, port, password, method, plugin, pluginopts);
}

void explodeSSD(std::string link, std::vector<Proxy> &nodes)
{
    Document jsondata;
    Proxy node;
    uint32_t index = nodes.size(), listType = 0, listCount = 0;
    std::string group, port, method, password, server, remarks;
    std::string plugin, pluginopts;
    std::map<uint32_t, std::string> node_map;

    link = urlSafeBase64Decode(link.substr(6));
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
        uint32_t node_index = 0;
        for(rapidjson::Value::MemberIterator iter = jsondata["servers"].MemberBegin(); iter != jsondata["servers"].MemberEnd(); iter++)
        {
            node_map.emplace(node_index, iter->name.GetString());
            node_index++;
        }
    }
    else
        return;

    rapidjson::Value singlenode;
    for(uint32_t i = 0; i < listCount; i++)
    {
        //get default info
        port = GetMember(jsondata, "port");
        method = GetMember(jsondata, "encryption");
        password = GetMember(jsondata, "password");
        plugin = GetMember(jsondata, "plugin");
        pluginopts = GetMember(jsondata, "plugin_options");

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

        if(port == "0")
            continue;

        ssConstruct(node, group, remarks, server, port, password, method, plugin, pluginopts);
        node.Id = index;
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
    return;
}

void explodeSSAndroid(std::string ss, std::vector<Proxy> &nodes)
{
    std::string ps, password, method, server, port, group = SS_DEFAULT_GROUP;
    std::string plugin, pluginopts;

    Document json;
    Proxy node;
    int index = nodes.size();
    //first add some extra data before parsing
    ss = "{\"nodes\":" + ss + "}";
    json.Parse(ss.data());
    if(json.HasParseError())
        return;

    for(uint32_t i = 0; i < json["nodes"].Size(); i++)
    {
        server = GetMember(json["nodes"][i], "server");
        if(server.empty())
            continue;
        ps = GetMember(json["nodes"][i], "remarks");
        port = GetMember(json["nodes"][i], "server_port");
        if(port == "0")
            continue;
        if(ps.empty())
            ps = server + ":" + port;
        password = GetMember(json["nodes"][i], "password");
        method = GetMember(json["nodes"][i], "method");
        plugin = GetMember(json["nodes"][i], "plugin");
        pluginopts = GetMember(json["nodes"][i], "plugin_opts");

        ssConstruct(node, group, ps, server, port, password, method, plugin, pluginopts);
        node.Id = index;
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
}

void explodeSSConf(std::string content, std::vector<Proxy> &nodes)
{
    Proxy node;
    Document json;
    std::string ps, password, method, server, port, plugin, pluginopts, group = SS_DEFAULT_GROUP;
    int index = nodes.size();

    json.Parse(content.data());
    if(json.HasParseError())
        return;
    const char *section = json.HasMember("version") && json.HasMember("servers") ? "servers" : "configs";
    if(!json.HasMember(section))
        return;
    GetMember(json, "remarks", group);

    for(uint32_t i = 0; i < json[section].Size(); i++)
    {
        ps = GetMember(json[section][i], "remarks");
        port = GetMember(json[section][i], "server_port");
        if(port == "0")
            continue;
        if(ps.empty())
            ps = server + ":" + port;

        password = GetMember(json[section][i], "password");
        method = GetMember(json[section][i], "method");
        server = GetMember(json[section][i], "server");
        plugin = GetMember(json[section][i], "plugin");
        pluginopts = GetMember(json[section][i], "plugin_opts");

        node.Id = index;
        ssConstruct(node, group, ps, server, port, password, method, plugin, pluginopts);
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
    return;
}

void explodeSSR(std::string ssr, Proxy &node)
{
    std::string strobfs;
    std::string remarks, group, server, port, method, password, protocol, protoparam, obfs, obfsparam;
    ssr = replaceAllDistinct(ssr.substr(6), "\r", "");
    ssr = urlSafeBase64Decode(ssr);
    if(strFind(ssr, "/?"))
    {
        strobfs = ssr.substr(ssr.find("/?") + 2);
        ssr = ssr.substr(0, ssr.find("/?"));
        group = urlSafeBase64Decode(getUrlArg(strobfs, "group"));
        remarks = urlSafeBase64Decode(getUrlArg(strobfs, "remarks"));
        obfsparam = regReplace(urlSafeBase64Decode(getUrlArg(strobfs, "obfsparam")), "\\s", "");
        protoparam = regReplace(urlSafeBase64Decode(getUrlArg(strobfs, "protoparam")), "\\s", "");
    }

    if(regGetMatch(ssr, "(\\S+):(\\d+?):(\\S+?):(\\S+?):(\\S+?):(\\S+)", 7, 0, &server, &port, &protocol, &method, &obfs, &password))
        return;
    password = urlSafeBase64Decode(password);
    if(port == "0")
        return;

    if(group.empty())
        group = SSR_DEFAULT_GROUP;
    if(remarks.empty())
        remarks = server + ":" + port;

    if(find(ss_ciphers.begin(), ss_ciphers.end(), method) != ss_ciphers.end() && (obfs.empty() || obfs == "plain") && (protocol.empty() || protocol == "origin"))
    {
        ssConstruct(node, group, remarks, server, port, password, method, "", "");
    }
    else
    {
        ssrConstruct(node, group, remarks, server, port, protocol, method, obfs, password, obfsparam, protoparam);
    }
}

void explodeSSRConf(std::string content, std::vector<Proxy> &nodes)
{
    Proxy node;
    Document json;
    std::string remarks, group, server, port, method, password, protocol, protoparam, obfs, obfsparam, plugin, pluginopts;
    int index = nodes.size();

    json.Parse(content.data());
    if(json.HasParseError())
        return;

    if(json.HasMember("local_port") && json.HasMember("local_address")) //single libev config
    {
        server = GetMember(json, "server");
        port = GetMember(json, "server_port");
        remarks = server + ":" + port;
        method = GetMember(json, "method");
        obfs = GetMember(json, "obfs");
        protocol = GetMember(json, "protocol");
        if(find(ss_ciphers.begin(), ss_ciphers.end(), method) != ss_ciphers.end() && (obfs.empty() || obfs == "plain") && (protocol.empty() || protocol == "origin"))
        {
            plugin = GetMember(json, "plugin");
            pluginopts = GetMember(json, "plugin_opts");
            ssConstruct(node, SS_DEFAULT_GROUP, remarks, server, port, password, method, plugin, pluginopts);
        }
        else
        {
            protoparam = GetMember(json, "protocol_param");
            obfsparam = GetMember(json, "obfs_param");
            ssrConstruct(node, SSR_DEFAULT_GROUP, remarks, server, port, protocol, method, obfs, password, obfsparam, protoparam);
        }
        nodes.emplace_back(std::move(node));
        node = Proxy();
        return;
    }

    for(uint32_t i = 0; i < json["configs"].Size(); i++)
    {
        group = GetMember(json["configs"][i], "group");
        if(group.empty())
            group = SSR_DEFAULT_GROUP;
        remarks = GetMember(json["configs"][i], "remarks");
        server = GetMember(json["configs"][i], "server");
        port = GetMember(json["configs"][i], "server_port");
        if(port == "0")
            continue;
        if(remarks.empty())
            remarks = server + ":" + port;

        password = GetMember(json["configs"][i], "password");
        method = GetMember(json["configs"][i], "method");

        protocol = GetMember(json["configs"][i], "protocol");
        protoparam = GetMember(json["configs"][i], "protocolparam");
        obfs = GetMember(json["configs"][i], "obfs");
        obfsparam = GetMember(json["configs"][i], "obfsparam");

        ssrConstruct(node, group, remarks, server, port, protocol, method, obfs, password, obfsparam, protoparam);
        node.Id = index;
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
    return;
}

void explodeSocks(std::string link, Proxy &node)
{
    std::string group, remarks, server, port, username, password;
    if(strFind(link, "socks://")) //v2rayn socks link
    {
        std::vector<std::string> arguments;
        if(strFind(link, "#"))
        {
            remarks = urlDecode(link.substr(link.find("#") + 1));
            link.erase(link.find("#"));
        }
        link = urlSafeBase64Decode(link.substr(8));
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
        username = urlDecode(getUrlArg(link, "user"));
        password = urlDecode(getUrlArg(link, "pass"));
        remarks = urlDecode(getUrlArg(link, "remarks"));
        group = urlDecode(getUrlArg(link, "group"));
    }
    if(group.empty())
        group = SOCKS_DEFAULT_GROUP;
    if(remarks.empty())
        remarks = server + ":" + port;
    if(port == "0")
        return;

    socksConstruct(node, group, remarks, server, port, username, password);
}

void explodeHTTP(const std::string &link, Proxy &node)
{
    std::string group, remarks, server, port, username, password;
    server = getUrlArg(link, "server");
    port = getUrlArg(link, "port");
    username = urlDecode(getUrlArg(link, "user"));
    password = urlDecode(getUrlArg(link, "pass"));
    remarks = urlDecode(getUrlArg(link, "remarks"));
    group = urlDecode(getUrlArg(link, "group"));

    if(group.empty())
        group = HTTP_DEFAULT_GROUP;
    if(remarks.empty())
        remarks = server + ":" + port;
    if(port == "0")
        return;

    httpConstruct(node, group, remarks, server, port, username, password, strFind(link, "/https"));
}

void explodeHTTPSub(std::string link, Proxy &node)
{
    std::string group, remarks, server, port, username, password;
    std::string addition;
    bool tls = strFind(link, "https://");
    string_size pos = link.find("?");
    if(pos != link.npos)
    {
        addition = link.substr(pos + 1);
        link.erase(pos);
        remarks = urlDecode(getUrlArg(addition, "remarks"));
        group = urlDecode(getUrlArg(addition, "group"));
    }
    link.erase(0, link.find("://") + 3);
    link = urlSafeBase64Decode(link);
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
    if(port == "0")
        return;

    httpConstruct(node, group, remarks, server, port, username, password, tls);
}

void explodeTrojan(std::string trojan, Proxy &node)
{
    std::string server, port, psk, addition, group, remark, host, path, network;
    tribool tfo, scv;
    trojan.erase(0, 9);
    string_size pos = trojan.rfind("#");

    if(pos != trojan.npos)
    {
        remark = urlDecode(trojan.substr(pos + 1));
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
    if(port == "0")
        return;

    host = getUrlArg(addition, "peer");
    tfo = getUrlArg(addition, "tfo");
    scv = getUrlArg(addition, "allowInsecure");
    group = urlDecode(getUrlArg(addition, "group"));

    if(getUrlArg(addition, "ws") == "1")
    {
        path = getUrlArg(addition, "wspath");
        network = "ws";
    }

    if(remark.empty())
        remark = server + ":" + port;
    if(host.empty() && !isIPv4(server) && !isIPv6(server))
        host = server;
    if(group.empty())
        group = TROJAN_DEFAULT_GROUP;

    trojanConstruct(node, group, remark, server, port, psk, network, host, path, true, tribool(), tfo, scv);
}

void explodeQuan(const std::string &quan, Proxy &node)
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
        port = trim(configs[3]);
        if(port == "0")
            return;
        cipher = trim(configs[4]);
        id = trim(replaceAllDistinct(configs[5], "\"", ""));

        //read link
        for(uint32_t i = 6; i < configs.size(); i++)
        {
            vArray = split(configs[i], "=");
            if(vArray.size() < 2)
                continue;
            itemName = trim(vArray[0]);
            itemVal = trim(vArray[1]);
            switch(hash_(itemName))
            {
            case "group"_hash:
                group = itemVal;
                break;
            case "over-tls"_hash:
                tls = itemVal == "true" ? "tls" : "";
                break;
            case "tls-host"_hash:
                host = itemVal;
                break;
            case "obfs-path"_hash:
                path = replaceAllDistinct(itemVal, "\"", "");
                break;
            case "obfs-header"_hash:
                headers = split(replaceAllDistinct(replaceAllDistinct(itemVal, "\"", ""), "[Rr][Nn]", "|"), "|");
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
            default:
                continue;
            }
        }
        if(path.empty())
            path = "/";

        vmessConstruct(node, group, ps, add, port, type, id, aid, net, cipher, path, host, edge, tls);
    }
}

void explodeNetch(std::string netch, Proxy &node)
{
    Document json;
    std::string type, group, remark, address, port, username, password, method, plugin, pluginopts, protocol, protoparam, obfs, obfsparam, id, aid, transprot, faketype, host, edge, path, tls;
    tribool udp, tfo, scv;
    netch = urlSafeBase64Decode(netch.substr(8));

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
    port = GetMember(json, "Port");
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
        ssConstruct(node, group, remark, address, port, password, method, plugin, pluginopts, udp, tfo, scv);
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
            ssConstruct(node, group, remark, address, port, password, method, plugin, pluginopts, udp, tfo, scv);
        }
        else
        {
            protoparam = GetMember(json, "ProtocolParam");
            obfsparam = GetMember(json, "OBFSParam");
            if(group.empty())
                group = SSR_DEFAULT_GROUP;
            ssrConstruct(node, group, remark, address, port, protocol, method, obfs, password, obfsparam, protoparam, udp, tfo, scv);
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
        if(group.empty())
            group = V2RAY_DEFAULT_GROUP;
        vmessConstruct(node, group, remark, address, port, faketype, id, aid, transprot, method, path, host, edge, tls, udp, tfo, scv);
        break;
    case "Socks5"_hash:
        username = GetMember(json, "Username");
        if(group.empty())
            group = SOCKS_DEFAULT_GROUP;
        socksConstruct(node, group, remark, address, port, username, password, udp, tfo, scv);
        break;
    case "HTTP"_hash:
    case "HTTPS"_hash:
        if(group.empty())
            group = HTTP_DEFAULT_GROUP;
        httpConstruct(node, group, remark, address, port, username, password, type == "HTTPS", tfo, scv);
        break;
    case "Trojan"_hash:
        host = GetMember(json, "Host");
        path = GetMember(json, "Path");
        transprot = GetMember(json, "TransferProtocol");
        tls = GetMember(json, "TLSSecure");
        if(group.empty())
            group = TROJAN_DEFAULT_GROUP;
        trojanConstruct(node, group, remark, address, port, password, transprot, host, path, tls == "true", udp, tfo, scv);
        break;
    case "Snell"_hash:
        obfs = GetMember(json, "OBFS");
        host = GetMember(json, "Host");
        if(group.empty())
            group = SNELL_DEFAULT_GROUP;
        snellConstruct(node, group, remark, address, port, password, obfs, host, udp, tfo, scv);
        break;
    default:
        return;
    }
}

void explodeClash(Node yamlnode, std::vector<Proxy> &nodes)
{
    std::string proxytype, ps, server, port, cipher, group, password; //common
    std::string type = "none", id, aid = "0", net = "tcp", path, host, edge, tls; //vmess
    std::string plugin, pluginopts, pluginopts_mode, pluginopts_host, pluginopts_mux; //ss
    std::string protocol, protoparam, obfs, obfsparam; //ssr
    std::string user; //socks
    tribool udp, tfo, scv;
    Proxy node;
    Node singleproxy;
    uint32_t index = nodes.size();
    const std::string section = yamlnode["proxies"].IsDefined() ? "proxies" : "Proxy";
    for(uint32_t i = 0; i < yamlnode[section].size(); i++)
    {
        singleproxy = yamlnode[section][i];
        singleproxy["type"] >>= proxytype;
        singleproxy["name"] >>= ps;
        singleproxy["server"] >>= server;
        singleproxy["port"] >>= port;
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
            switch(hash_(net))
            {
            case "http"_hash:
                singleproxy["http-opts"]["path"][0] >>= path;
                singleproxy["http-opts"]["headers"]["Host"][0] >>= host;
                edge.clear();
                break;
            case "ws"_hash:
                if(singleproxy["ws-opts"].IsDefined())
                {
                    path = singleproxy["ws-opts"]["path"].IsDefined() ? safe_as<std::string>(singleproxy["ws-opts"]["path"]) : "/";
                    singleproxy["ws-opts"]["headers"]["Host"] >>= host;
                    singleproxy["ws-opts"]["headers"]["Edge"] >>= edge;
                }
                else
                {
                    path = singleproxy["ws-path"].IsDefined() ? safe_as<std::string>(singleproxy["ws-path"]) : "/";
                    singleproxy["ws-headers"]["Host"] >>= host;
                    singleproxy["ws-headers"]["Edge"] >>= edge;
                }
                break;
            case "h2"_hash:
                singleproxy["h2-opts"]["path"] >>= path;
                singleproxy["h2-opts"]["host"][0] >>= host;
                edge.clear();
                break;
            case "grpc"_hash:
                singleproxy["servername"] >>= host;
                singleproxy["grpc-opts"]["grpc-service-name"] >>= path;
                edge.clear();
                break;
            }
            tls = safe_as<std::string>(singleproxy["tls"]) == "true" ? "tls" : "";

            vmessConstruct(node, group, ps, server, port, "", id, aid, net, cipher, path, host, edge, tls, udp, tfo, scv);
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
                plugin = "obfs-local";
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
                if(!pluginopts_host.empty())
                    pluginopts += "host=" + pluginopts_host + ";";
                if(!path.empty())
                    pluginopts += "path=" + path + ";";
                if(!pluginopts_mux.empty())
                    pluginopts += "mux=" + pluginopts_mux + ";";
                break;
            }

            //support for go-shadowsocks2
            if(cipher == "AEAD_CHACHA20_POLY1305")
                cipher = "chacha20-ietf-poly1305";
            else if(strFind(cipher, "AEAD"))
            {
                cipher = replaceAllDistinct(replaceAllDistinct(cipher, "AEAD_", ""), "_", "-");
                std::transform(cipher.begin(), cipher.end(), cipher.begin(), ::tolower);
            }

            ssConstruct(node, group, ps, server, port, password, cipher, plugin, pluginopts, udp, tfo, scv);
            break;
        case "socks5"_hash:
            group = SOCKS_DEFAULT_GROUP;

            singleproxy["username"] >>= user;
            singleproxy["password"] >>= password;

            socksConstruct(node, group, ps, server, port, user, password);
            break;
        case "ssr"_hash:
            group = SSR_DEFAULT_GROUP;

            singleproxy["cipher"] >>= cipher;
            if(cipher == "dummy") cipher = "none";
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

            ssrConstruct(node, group, ps, server, port, protocol, cipher, obfs, password, obfsparam, protoparam, udp, tfo, scv);
            break;
        case "http"_hash:
            group = HTTP_DEFAULT_GROUP;

            singleproxy["username"] >>= user;
            singleproxy["password"] >>= password;
            singleproxy["tls"] >>= tls;

            httpConstruct(node, group, ps, server, port, user, password, tls == "true", tfo, scv);
            break;
        case "trojan"_hash:
            group = TROJAN_DEFAULT_GROUP;
            singleproxy["password"] >>= password;
            singleproxy["sni"] >>= host;
            singleproxy["network"] >>= net;
            switch(hash_(net))
            {
            case "grpc"_hash:
                singleproxy["grpc-opts"]["grpc-service-name"] >>= path;
                break;
            case "ws"_hash:
                singleproxy["ws-opts"]["path"] >>= path;
                break;
            default:
                net = "tcp";
                path.clear();
                break;
            }

            trojanConstruct(node, group, ps, server, port, password, net, host, path, true, udp, tfo, scv);
            break;
        case "snell"_hash:
            group = SNELL_DEFAULT_GROUP;
            singleproxy["psk"] >> password;
            singleproxy["obfs-opts"]["mode"] >>= obfs;
            singleproxy["obfs-opts"]["host"] >>= host;

            snellConstruct(node, group, ps, server, port, password, obfs, host, udp, tfo, scv);
            break;
        default:
            continue;
        }

        node.Id = index;
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
    return;
}

void explodeStdVMess(std::string vmess, Proxy &node)
{
    std::string add, port, type, id, aid, net, path, host, tls, remarks;
    std::string addition;
    vmess = vmess.substr(8);
    string_size pos;

    pos = vmess.rfind("#");
    if(pos != vmess.npos)
    {
        remarks = urlDecode(vmess.substr(pos + 1));
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

    if(remarks.empty())
        remarks = add + ":" + port;

    vmessConstruct(node, V2RAY_DEFAULT_GROUP, remarks, add, port, type, id, aid, net, "auto", path, host, "", tls);
    return;
}

void explodeShadowrocket(std::string rocket, Proxy &node)
{
    std::string add, port, type, id, aid, net = "tcp", path, host, tls, cipher, remarks;
    std::string obfs; //for other style of link
    std::string addition;
    rocket = rocket.substr(8);

    string_size pos = rocket.find("?");
    addition = rocket.substr(pos + 1);
    rocket.erase(pos);

    if(regGetMatch(urlSafeBase64Decode(rocket), "(.*?):(.*)@(.*):(.*)", 5, 0, &cipher, &id, &add, &port))
        return;
    if(port == "0")
        return;
    remarks = urlDecode(getUrlArg(addition, "remarks"));
    obfs = getUrlArg(addition, "obfs");
    if(!obfs.empty())
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

    vmessConstruct(node, V2RAY_DEFAULT_GROUP, remarks, add, port, type, id, aid, net, cipher, path, host, "", tls);
}

void explodeKitsunebi(std::string kit, Proxy &node)
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
    if(port == "0")
        return;
    net = getUrlArg(addition, "network");
    tls = getUrlArg(addition, "tls") == "true" ? "tls" : "";
    host = getUrlArg(addition, "ws.host");

    if(remarks.empty())
        remarks = add + ":" + port;

    vmessConstruct(node, V2RAY_DEFAULT_GROUP, remarks, add, port, type, id, aid, net, cipher, path, host, "", tls);
}

bool explodeSurge(std::string surge, std::vector<Proxy> &nodes)
{
    std::multimap<std::string, std::string> proxies;
    Proxy node;
    uint32_t i, index = nodes.size();
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
            port = trim(configs[2]);
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
                case "obfs-host"_hash:
                    pluginopts_host = itemVal;
                    break;
                case "udp-relay"_hash:
                    udp = itemVal;
                    break;
                case "tfo"_hash:
                    tfo = itemVal;
                    break;
                default:
                    continue;
                }
            }
            if(!plugin.empty())
            {
                pluginopts = "obfs=" + pluginopts_mode;
                pluginopts += pluginopts_host.empty() ? "" : ";obfs-host=" + pluginopts_host;
            }

            ssConstruct(node, SS_DEFAULT_GROUP, remarks, server, port, password, method, plugin, pluginopts, udp, tfo, scv);
        }
            //else
            //    continue;
        break;
        case "ss"_hash: //surge 3 style ss proxy
            server = trim(configs[1]);
            port = trim(configs[2]);
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
                case "encrypt-method"_hash:
                    method = itemVal;
                    break;
                case "password"_hash:
                    password = itemVal;
                    break;
                case "obfs"_hash:
                    plugin = "simple-obfs";
                    pluginopts_mode = itemVal;
                    break;
                case "obfs-host"_hash:
                    pluginopts_host = itemVal;
                    break;
                case "udp-relay"_hash:
                    udp = itemVal;
                    break;
                case "tfo"_hash:
                    tfo = itemVal;
                    break;
                default:
                    continue;
                }
            }
            if(!plugin.empty())
            {
                pluginopts = "obfs=" + pluginopts_mode;
                pluginopts += pluginopts_host.empty() ? "" : ";obfs-host=" + pluginopts_host;
            }

            ssConstruct(node, SS_DEFAULT_GROUP, remarks, server, port, password, method, plugin, pluginopts, udp, tfo, scv);
            break;
        case "socks5"_hash: //surge 3 style socks5 proxy
            server = trim(configs[1]);
            port = trim(configs[2]);
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
                case "udp-relay"_hash:
                    udp = itemVal;
                    break;
                case "tfo"_hash:
                    tfo = itemVal;
                    break;
                case "skip-cert-verify"_hash:
                    scv = itemVal;
                    break;
                default:
                    continue;
                }
            }
            socksConstruct(node, SOCKS_DEFAULT_GROUP, remarks, server, port, username, password, udp, tfo, scv);
            break;
        case "vmess"_hash: //surge 4 style vmess proxy
            server = trim(configs[1]);
            port = trim(configs[2]);
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
                case "username"_hash:
                    id = itemVal;
                    break;
                case "ws"_hash:
                    net = itemVal == "true" ? "ws" : "tcp";
                    break;
                case "tls"_hash:
                    tls = itemVal == "true" ? "tls" : "";
                    break;
                case "ws-path"_hash:
                    path = itemVal;
                    break;
                case "obfs-host"_hash:
                    host = itemVal;
                    break;
                case "ws-headers"_hash:
                    headers = split(itemVal, "|");
                    for(auto &y : headers)
                    {
                        header = split(trim(y), ":");
                        if(header.size() != 2)
                            continue;
                        else if(regMatch(header[0], "(?i)host"))
                            host = trimQuote(header[1]);
                        else if(regMatch(header[0], "(?i)edge"))
                            edge = trimQuote(header[1]);
                    }
                    break;
                case "udp-relay"_hash:
                    udp = itemVal;
                    break;
                case "tfo"_hash:
                    tfo = itemVal;
                    break;
                case "skip-cert-verify"_hash:
                    scv = itemVal;
                    break;
                case "tls13"_hash:
                    tls13 = itemVal;
                    break;
                default:
                    continue;
                }
            }
            if(host.empty() && !isIPv4(server) && !isIPv6(server))
                host = server;

            vmessConstruct(node, V2RAY_DEFAULT_GROUP, remarks, server, port, "", id, "0", net, method, path, host, edge, tls, udp, tfo, scv, tls13);
            break;
        case "http"_hash: //http proxy
            server = trim(configs[1]);
            port = trim(configs[2]);
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
                case "username"_hash:
                    username = itemVal;
                    break;
                case "password"_hash:
                    password = itemVal;
                    break;
                case "skip-cert-verify"_hash:
                    scv = itemVal;
                    break;
                default:
                    continue;
                }
            }
            httpConstruct(node, HTTP_DEFAULT_GROUP, remarks, server, port, username, password, false, tfo, scv);
            break;
        case "trojan"_hash: // surge 4 style trojan proxy
            server = trim(configs[1]);
            port = trim(configs[2]);
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
                case "password"_hash:
                    password = itemVal;
                    break;
                case "sni"_hash:
                    host = itemVal;
                    break;
                case "udp-relay"_hash:
                    udp = itemVal;
                    break;
                case "tfo"_hash:
                    tfo = itemVal;
                    break;
                case "skip-cert-verify"_hash:
                    scv = itemVal;
                    break;
                default:
                    continue;
                }
            }
            if(host.empty() && !isIPv4(server) && !isIPv6(server))
                host = server;

            trojanConstruct(node, TROJAN_DEFAULT_GROUP, remarks, server, port, password, "", host, "", true, udp, tfo, scv);
            break;
        case "snell"_hash:
            server = trim(configs[1]);
            port = trim(configs[2]);
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
                case "psk"_hash:
                    password = itemVal;
                    break;
                case "obfs"_hash:
                    plugin = itemVal;
                    break;
                case "obfs-host"_hash:
                    host = itemVal;
                    break;
                case "udp-relay"_hash:
                    udp = itemVal;
                    break;
                case "tfo"_hash:
                    tfo = itemVal;
                    break;
                case "skip-cert-verify"_hash:
                    scv = itemVal;
                    break;
                default:
                    continue;
                }
            }
            if(host.empty() && !isIPv4(server) && !isIPv6(server))
                host = server;

            snellConstruct(node, SNELL_DEFAULT_GROUP, remarks, server, port, password, plugin, host, udp, tfo, scv);
            break;
        default:
            switch(hash_(remarks))
            {
            case "shadowsocks"_hash: //quantumult x style ss/ssr link
                server = trim(configs[0].substr(0, configs[0].rfind(":")));
                port = trim(configs[0].substr(configs[0].rfind(":") + 1));
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
                    case "method"_hash:
                        method = itemVal;
                        break;
                    case "password"_hash:
                        password = itemVal;
                        break;
                    case "tag"_hash:
                        remarks = itemVal;
                        break;
                    case "ssr-protocol"_hash:
                        protocol = itemVal;
                        break;
                    case "ssr-protocol-param"_hash:
                        protoparam = itemVal;
                        break;
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
                    case "obfs-host"_hash:
                        pluginopts_host = itemVal;
                        break;
                    case "obfs-uri"_hash:
                        path = itemVal;
                        break;
                    case "udp-relay"_hash:
                        udp = itemVal;
                        break;
                    case "fast-open"_hash:
                        tfo = itemVal;
                        break;
                    case "tls13"_hash:
                        tls13 = itemVal;
                        break;
                    default:
                        continue;
                    }
                }
                if(remarks.empty())
                    remarks = server + ":" + port;
                switch(hash_(plugin))
                {
                case "simple-obfs"_hash:
                    pluginopts = "obfs=" + pluginopts_mode;
                    if(!pluginopts_host.empty())
                        pluginopts += ";obfs-host=" + pluginopts_host;
                    break;
                case "v2ray-plugin"_hash:
                    if(pluginopts_host.empty() && !isIPv4(server) && !isIPv6(server))
                        pluginopts_host = server;
                    pluginopts = "mode=" + pluginopts_mode;
                    if(!pluginopts_host.empty())
                        pluginopts += ";host=" + pluginopts_host;
                    if(!path.empty())
                        pluginopts += ";path=" + path;
                    pluginopts += ";" + tls;
                    break;
                }

                if(!protocol.empty())
                {
                    ssrConstruct(node, SSR_DEFAULT_GROUP, remarks, server, port, protocol, method, pluginopts_mode, password, pluginopts_host, protoparam, udp, tfo, scv);
                }
                else
                {
                    ssConstruct(node, SS_DEFAULT_GROUP, remarks, server, port, password, method, plugin, pluginopts, udp, tfo, scv, tls13);
                }
                break;
            case "vmess"_hash: //quantumult x style vmess link
                server = trim(configs[0].substr(0, configs[0].rfind(":")));
                port = trim(configs[0].substr(configs[0].rfind(":") + 1));
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
                    case "method"_hash:
                        method = itemVal;
                        break;
                    case "password"_hash:
                        id = itemVal;
                        break;
                    case "tag"_hash:
                        remarks = itemVal;
                        break;
                    case "obfs"_hash:
                        switch(hash_(itemVal))
                        {
                        case "ws"_hash:
                            net = "ws";
                            break;
                        case "over-tls"_hash:
                            tls = "tls";
                            break;
                        case "wss"_hash:
                            net = "ws";
                            tls = "tls";
                            break;
                        }
                        break;
                    case "obfs-host"_hash:
                        host = itemVal;
                        break;
                    case "obfs-uri"_hash:
                        path = itemVal;
                        break;
                    case "over-tls"_hash:
                        tls = itemVal == "true" ? "tls" : "";
                        break;
                    case "udp-relay"_hash:
                        udp = itemVal;
                        break;
                    case "fast-open"_hash:
                        tfo = itemVal;
                        break;
                    case "tls13"_hash:
                        tls13 = itemVal;
                        break;
                    default:
                        continue;
                    }
                }
                if(remarks.empty())
                    remarks = server + ":" + port;

                if(host.empty() && !isIPv4(server) && !isIPv6(server))
                    host = server;

                vmessConstruct(node, V2RAY_DEFAULT_GROUP, remarks, server, port, "", id, "0", net, method, path, host, "", tls, udp, tfo, scv, tls13);
                break;
            case "trojan"_hash: //quantumult x style trojan link
                server = trim(configs[0].substr(0, configs[0].rfind(":")));
                port = trim(configs[0].substr(configs[0].rfind(":") + 1));
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
                    case "password"_hash:
                        password = itemVal;
                        break;
                    case "tag"_hash:
                        remarks = itemVal;
                        break;
                    case "over-tls"_hash:
                        tls = itemVal;
                        break;
                    case "tls-host"_hash:
                        host = itemVal;
                        break;
                    case "udp-relay"_hash:
                        udp = itemVal;
                        break;
                    case "fast-open"_hash:
                        tfo = itemVal;
                        break;
                    case "tls-verification"_hash:
                        scv = itemVal == "false";
                        break;
                    case "tls13"_hash:
                        tls13 = itemVal;
                        break;
                    default:
                        continue;
                    }
                }
                if(remarks.empty())
                    remarks = server + ":" + port;

                if(host.empty() && !isIPv4(server) && !isIPv6(server))
                    host = server;

                trojanConstruct(node, TROJAN_DEFAULT_GROUP, remarks, server, port, password, "", host, "", tls == "true", udp, tfo, scv, tls13);
                break;
            case "http"_hash: //quantumult x style http links
                server = trim(configs[0].substr(0, configs[0].rfind(":")));
                port = trim(configs[0].substr(configs[0].rfind(":") + 1));
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
                    case "username"_hash:
                        username = itemVal;
                        break;
                    case "password"_hash:
                        password = itemVal;
                        break;
                    case "tag"_hash:
                        remarks = itemVal;
                        break;
                    case "over-tls"_hash:
                        tls = itemVal;
                        break;
                    case "tls-verification"_hash:
                        scv = itemVal == "false";
                        break;
                    case "tls13"_hash:
                        tls13 = itemVal;
                        break;
                    case "fast-open"_hash:
                        tfo = itemVal;
                        break;
                    default:
                        continue;
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

                httpConstruct(node, HTTP_DEFAULT_GROUP, remarks, server, port, username, password, tls == "true", tfo, scv, tls13);
                break;
            default:
                continue;
            }
            break;
        }

        node.Id = index;
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
    return index;
}

void explodeSSTap(std::string sstap, std::vector<Proxy> &nodes)
{
    std::string configType, group, remarks, server, port;
    std::string cipher;
    std::string user, pass;
    std::string protocol, protoparam, obfs, obfsparam;
    Document json;
    Proxy node;
    uint32_t index = nodes.size();
    json.Parse(sstap.data());
    if(json.HasParseError())
        return;

    for(uint32_t i = 0; i < json["configs"].Size(); i++)
    {
        json["configs"][i]["group"] >> group;
        json["configs"][i]["remarks"] >> remarks;
        json["configs"][i]["server"] >> server;
        port = GetMember(json["configs"][i], "server_port");
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
            socksConstruct(node, group, remarks, server, port, user, pass);
            break;
        case 6: //ss/ssr
            json["configs"][i]["protocol"] >> protocol;
            json["configs"][i]["obfs"] >> obfs;
            json["configs"][i]["method"] >> cipher;
            if(find(ss_ciphers.begin(), ss_ciphers.end(), cipher) != ss_ciphers.end() && protocol == "origin" && obfs == "plain") //is ss
            {
                ssConstruct(node, group, remarks, server, port, pass, cipher, "", "");
            }
            else //is ssr cipher
            {
                json["configs"][i]["obfsparam"] >> obfsparam;
                json["configs"][i]["protocolparam"] >> protoparam;
                ssrConstruct(node, group, remarks, base64Encode(remarks), server, port, protocol, cipher, obfs, pass, obfsparam, protoparam);
            }
            break;
        default:
            continue;
        }

        node.Id = index;
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
}

void explodeNetchConf(std::string netch, std::vector<Proxy> &nodes)
{
    Document json;
    Proxy node;
    uint32_t index = nodes.size();

    json.Parse(netch.data());
    if(json.HasParseError())
        return;

    if(!json.HasMember("Server"))
        return;

    for(uint32_t i = 0; i < json["Server"].Size(); i++)
    {
        explodeNetch("Netch://" + base64Encode(SerializeObject(json["Server"][i])), node);

        node.Id = index;
        nodes.emplace_back(std::move(node));
        node = Proxy();
        index++;
    }
}

int explodeConfContent(const std::string &content, std::vector<Proxy> &nodes)
{
    ConfType filetype = ConfType::Unknow;

    if(strFind(content, "\"version\""))
        filetype = ConfType::SS;
    else if(strFind(content, "\"serverSubscribes\""))
        filetype = ConfType::SSR;
    else if(strFind(content, "\"uiItem\"") || strFind(content, "vnext"))
        filetype = ConfType::V2Ray;
    else if(strFind(content, "\"proxy_apps\""))
        filetype = ConfType::SSConf;
    else if(strFind(content, "\"idInUse\""))
        filetype = ConfType::SSTap;
    else if(strFind(content, "\"local_address\"") && strFind(content, "\"local_port\""))
        filetype = ConfType::SSR; //use ssr config parser
    else if(strFind(content, "\"ModeFileNameType\""))
        filetype = ConfType::Netch;

    switch(filetype)
    {
    case ConfType::SS:
        explodeSSConf(content, nodes);
        break;
    case ConfType::SSR:
        explodeSSRConf(content, nodes);
        break;
    case ConfType::V2Ray:
        explodeVmessConf(content, nodes);
        break;
    case ConfType::SSConf:
        explodeSSAndroid(content, nodes);
        break;
    case ConfType::SSTap:
        explodeSSTap(content, nodes);
        break;
    case ConfType::Netch:
        explodeNetchConf(content, nodes);
        break;
    default:
        //try to parse as a local subscription
        explodeSub(content, nodes);
    }

    return !nodes.empty();
}

void explode(const std::string &link, Proxy &node)
{
    // TODO: replace strFind with startsWith if appropriate
    if(strFind(link, "ssr://"))
        explodeSSR(link, node);
    else if(strFind(link, "vmess://") || strFind(link, "vmess1://"))
        explodeVmess(link, node);
    else if(strFind(link, "ss://"))
        explodeSS(link, node);
    else if(strFind(link, "socks://") || strFind(link, "https://t.me/socks") || strFind(link, "tg://socks"))
        explodeSocks(link, node);
    else if(strFind(link, "https://t.me/http") || strFind(link, "tg://http")) //telegram style http link
        explodeHTTP(link, node);
    else if(strFind(link, "Netch://"))
        explodeNetch(link, node);
    else if(strFind(link, "trojan://"))
        explodeTrojan(link, node);
    else if(isLink(link))
        explodeHTTPSub(link, node);
}

void explodeSub(std::string sub, std::vector<Proxy> &nodes)
{
    std::stringstream strstream;
    std::string strLink;
    bool processed = false;
    Proxy node;

    //try to parse as SSD configuration
    if(startsWith(sub, "ssd://"))
    {
        explodeSSD(sub, nodes);
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
                explodeClash(yamlnode, nodes);
                processed = true;
            }
        }
    }
    catch (std::exception &e)
    {
        //writeLog(0, e.what(), LOG_LEVEL_DEBUG);
        //ignore
        throw;
    }

    //try to parse as surge configuration
    if(!processed && explodeSurge(sub, nodes))
    {
        processed = true;
    }

    //try to parse as normal subscription
    if(!processed)
    {
        sub = urlSafeBase64Decode(sub);
        if(regFind(sub, "(vmess|shadowsocks|http|trojan)\\s*?="))
        {
            if(explodeSurge(sub, nodes))
                return;
        }
        strstream << sub;
        char delimiter = count(sub.begin(), sub.end(), '\n') < 1 ? count(sub.begin(), sub.end(), '\r') < 1 ? ' ' : '\r' : '\n';
        while(getline(strstream, strLink, delimiter))
        {
            if(strLink.rfind("\r") != strLink.npos)
                strLink.erase(strLink.size() - 1);
            explode(strLink, node);
            if(strLink.empty() || node.Type == ProxyType::Unknow)
            {
                continue;
            }
            nodes.emplace_back(std::move(node));
            node = Proxy();
        }
    }
}
