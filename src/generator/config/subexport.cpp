#include <algorithm>
#include <iostream>
#include <numeric>
#include <cmath>
#include <climits>

#include "../../config/regmatch.h"
#include "../../generator/config/subexport.h"
#include "../../generator/template/templates.h"
#include "../../handler/settings.h"
#include "../../parser/config/proxy.h"
#include "../../script/script_quickjs.h"
#include "../../utils/bitwise.h"
#include "../../utils/file_extra.h"
#include "../../utils/ini_reader/ini_reader.h"
#include "../../utils/logger.h"
#include "../../utils/network.h"
#include "../../utils/rapidjson_extra.h"
#include "../../utils/regexp.h"
#include "../../utils/stl_extra.h"
#include "../../utils/urlencode.h"
#include "../../utils/yamlcpp_extra.h"
#include "nodemanip.h"
#include "ruleconvert.h"

extern string_array ss_ciphers, ssr_ciphers;

const string_array clashr_protocols = {"origin", "auth_sha1_v4", "auth_aes128_md5", "auth_aes128_sha1", "auth_chain_a", "auth_chain_b"};
const string_array clashr_obfs = {"plain", "http_simple", "http_post", "random_head", "tls1.2_ticket_auth", "tls1.2_ticket_fastauth"};
const string_array clash_ssr_ciphers = {"rc4-md5", "aes-128-ctr", "aes-192-ctr", "aes-256-ctr", "aes-128-cfb", "aes-192-cfb", "aes-256-cfb", "chacha20-ietf", "xchacha20", "none"};

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
    writer.String(port.data());
    writer.Key("type");
    writer.String(type.empty() ? "none" : type.data());
    writer.Key("id");
    writer.String(id.data());
    writer.Key("aid");
    writer.String(aid.data());
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
            regGetMatch(x, reg_range, 3, 0, &range_begin_str, &range_end_str);
            range_begin = to_int(range_begin_str, INT_MAX);
            range_end = to_int(range_end_str, INT_MIN);
            if(target >= range_begin && target <= range_end)
                match = true;
        }
        else if(regMatch(x, reg_not))
        {
            match = true;
            if(to_int(regReplace(x, reg_not, "$1"), INT_MAX) == target)
                match = false;
        }
        else if(regMatch(x, reg_not_range))
        {
            match = true;
            regGetMatch(x, reg_range, 3, 0, &range_begin_str, &range_end_str);
            range_begin = to_int(range_begin_str, INT_MAX);
            range_end = to_int(range_end_str, INT_MIN);
            if(target >= range_begin && target <= range_end)
                match = false;
        }
        else if(regMatch(x, reg_less))
        {
            if(to_int(regReplace(x, reg_less, "$1"), INT_MAX) >= target)
                match = true;
        }
        else if(regMatch(x, reg_more))
        {
            if(to_int(regReplace(x, reg_more, "$1"), INT_MIN) <= target)
                match = true;
        }
    }
    return match;
}

bool applyMatcher(const std::string &rule, std::string &real_rule, const Proxy &node)
{
    std::string target, ret_real_rule;
    static const std::string groupid_regex = R"(^!!(?:GROUPID|INSERT)=([\d\-+!,]+)(?:!!(.*))?$)", group_regex = R"(^!!(?:GROUP)=(.+?)(?:!!(.*))?$)";
    static const std::string type_regex = R"(^!!(?:TYPE)=(.+?)(?:!!(.*))?$)", port_regex = R"(^!!(?:PORT)=(.+?)(?:!!(.*))?$)", server_regex = R"(^!!(?:SERVER)=(.+?)(?:!!(.*))?$)";
    static const string_array types = {"", "SS", "SSR", "VMESS", "TROJAN", "SNELL", "HTTP", "HTTPS", "SOCKS5"};
    if(startsWith(rule, "!!GROUP="))
    {
        regGetMatch(rule, group_regex, 3, 0, &target, &ret_real_rule);
        real_rule = ret_real_rule;
        return regFind(node.Group, target);
    }
    else if(startsWith(rule, "!!GROUPID=") || startsWith(rule, "!!INSERT="))
    {
        int dir = startsWith(rule, "!!INSERT=") ? -1 : 1;
        regGetMatch(rule, groupid_regex, 3, 0, &target, &ret_real_rule);
        real_rule = ret_real_rule;
        return matchRange(target, dir * node.GroupId);
    }
    else if(startsWith(rule, "!!TYPE="))
    {
        regGetMatch(rule, type_regex, 3, 0, &target, &ret_real_rule);
        real_rule = ret_real_rule;
        if(node.Type == ProxyType::Unknow)
            return false;
        return regMatch(types[node.Type], target);
    }
    else if(startsWith(rule, "!!PORT="))
    {
        regGetMatch(rule, port_regex, 3, 0, &target, &ret_real_rule);
        real_rule = ret_real_rule;
        return matchRange(target, node.Port);
    }
    else if(startsWith(rule, "!!SERVER="))
    {
        regGetMatch(rule, server_regex, 3, 0, &target, &ret_real_rule);
        real_rule = ret_real_rule;
        return regFind(node.Hostname, target);
    }
    else
        real_rule = rule;
    return true;
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

void groupGenerate(const std::string &rule, std::vector<Proxy> &nodelist, string_array &filtered_nodelist, bool add_direct, extra_settings &ext)
{
    std::string real_rule;
    if(startsWith(rule, "[]") && add_direct)
    {
        filtered_nodelist.emplace_back(rule.substr(2));
    }
#ifndef NO_JS_RUNTIME
    else if(startsWith(rule, "script:") && ext.authorized)
    {
        script_safe_runner(ext.js_runtime, ext.js_context, [&](qjs::Context &ctx){
            std::string script = fileGet(rule.substr(7), true);
            try
            {
                ctx.eval(script);
                auto filter = (std::function<std::string(const std::vector<Proxy>&)>) ctx.eval("filter");
                std::string result_list = filter(nodelist);
                filtered_nodelist = split(regTrim(result_list), "\n");
            }
            catch (qjs::exception)
            {
                script_print_stack(ctx);
            }
        }, global.scriptCleanContext);
    }
#endif // NO_JS_RUNTIME
    else
    {
        for(Proxy &x : nodelist)
        {
            if(applyMatcher(rule, real_rule, x) && (real_rule.empty() || regFind(x.Remark, real_rule)) && std::find(filtered_nodelist.begin(), filtered_nodelist.end(), x.Remark) == filtered_nodelist.end())
                filtered_nodelist.emplace_back(x.Remark);
        }
    }
}

void proxyToClash(std::vector<Proxy> &nodes, YAML::Node &yamlnode, const ProxyGroupConfigs &extra_proxy_group, bool clashR, extra_settings &ext)
{
    YAML::Node proxies, singleproxy, singlegroup, original_groups;
    std::vector<Proxy> nodelist;
    string_array remarks_list, filtered_nodelist;
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

    for(Proxy &x : nodes)
    {
        singleproxy.reset();

        std::string type = getProxyTypeName(x.Type);
        std::string remark, pluginopts = replaceAllDistinct(x.PluginOption, ";", "&");
        if(ext.append_proxy_type)
            x.Remark = "[" + type + "] " + x.Remark;

        processRemark(x.Remark, remark, remarks_list, false);

        tribool udp = ext.udp;
        tribool scv = ext.skip_cert_verify;
        udp.define(x.UDP);
        scv.define(x.AllowInsecure);

        singleproxy["name"] = remark;
        singleproxy["server"] = x.Hostname;
        singleproxy["port"] = x.Port;

        switch(x.Type)
        {
        case ProxyType::Shadowsocks:
            //latest clash core removed support for chacha20 encryption
            if(ext.filter_deprecated && x.EncryptMethod == "chacha20")
                continue;
            singleproxy["type"] = "ss";
            singleproxy["cipher"] = x.EncryptMethod;
            singleproxy["password"] = x.Password;
            if(std::all_of(x.Password.begin(), x.Password.end(), ::isdigit) && !x.Password.empty())
                singleproxy["password"].SetTag("str");
            switch(hash_(x.Plugin))
            {
            case "simple-obfs"_hash:
            case "obfs-local"_hash:
                singleproxy["plugin"] = "obfs";
                singleproxy["plugin-opts"]["mode"] = urlDecode(getUrlArg(pluginopts, "obfs"));
                singleproxy["plugin-opts"]["host"] = urlDecode(getUrlArg(pluginopts, "obfs-host"));
                break;
            case "v2ray-plugin"_hash:
                singleproxy["plugin"] = "v2ray-plugin";
                singleproxy["plugin-opts"]["mode"] = getUrlArg(pluginopts, "mode");
                singleproxy["plugin-opts"]["host"] = getUrlArg(pluginopts, "host");
                singleproxy["plugin-opts"]["path"] = getUrlArg(pluginopts, "path");
                singleproxy["plugin-opts"]["tls"] = pluginopts.find("tls") != std::string::npos;
                singleproxy["plugin-opts"]["mux"] = pluginopts.find("mux") != std::string::npos;
                if(!scv.is_undef())
                    singleproxy["plugin-opts"]["skip-cert-verify"] = scv.get();
                break;
            }
            break;
        case ProxyType::VMess:
            singleproxy["type"] = "vmess";
            singleproxy["uuid"] = x.UserId;
            singleproxy["alterId"] = x.AlterId;
            singleproxy["cipher"] = x.EncryptMethod;
            singleproxy["tls"] = x.TLSSecure;
            if(!scv.is_undef())
                singleproxy["skip-cert-verify"] = scv.get();
            if(!x.ServerName.empty())
                singleproxy["servername"] = x.ServerName;
            switch(hash_(x.TransferProtocol))
            {
            case "tcp"_hash:
                break;
            case "ws"_hash:
                singleproxy["network"] = x.TransferProtocol;
                if(ext.clash_new_field_name)
                {
                    singleproxy["ws-opts"]["path"] = x.Path;
                    if(!x.Host.empty())
                        singleproxy["ws-opts"]["headers"]["Host"] = x.Host;
                    if(!x.Edge.empty())
                        singleproxy["ws-opts"]["headers"]["Edge"] = x.Edge;
                }
                else
                {
                    singleproxy["ws-path"] = x.Path;
                    if(!x.Host.empty())
                        singleproxy["ws-headers"]["Host"] = x.Host;
                    if(!x.Edge.empty())
                        singleproxy["ws-headers"]["Edge"] = x.Edge;
                }
                break;
            case "http"_hash:
                singleproxy["network"] = x.TransferProtocol;
                singleproxy["http-opts"]["method"] = "GET";
                singleproxy["http-opts"]["path"].push_back(x.Path);
                if(!x.Host.empty())
                    singleproxy["http-opts"]["headers"]["Host"].push_back(x.Host);
                if(!x.Edge.empty())
                    singleproxy["http-opts"]["headers"]["Edge"].push_back(x.Edge);
                break;
            case "h2"_hash:
                singleproxy["network"] = x.TransferProtocol;
                singleproxy["h2-opts"]["path"] = x.Path;
                if(!x.Host.empty())
                    singleproxy["h2-opts"]["host"].push_back(x.Host);
                break;
            case "grpc"_hash:
                singleproxy["network"] = x.TransferProtocol;
                singleproxy["servername"] = x.Host;
                singleproxy["grpc-opts"]["grpc-service-name"] = x.Path;
                break;
            default:
                continue;
            }
            break;
        case ProxyType::ShadowsocksR:
            //ignoring all nodes with unsupported obfs, protocols and encryption
            if(ext.filter_deprecated)
            {
                if(!clashR && std::find(clash_ssr_ciphers.cbegin(), clash_ssr_ciphers.cend(), x.EncryptMethod) == clash_ssr_ciphers.cend())
                    continue;
                if(std::find(clashr_protocols.cbegin(), clashr_protocols.cend(), x.Protocol) == clashr_protocols.cend())
                    continue;
                if(std::find(clashr_obfs.cbegin(), clashr_obfs.cend(), x.OBFS) == clashr_obfs.cend())
                    continue;
            }

            singleproxy["type"] = "ssr";
            singleproxy["cipher"] = x.EncryptMethod == "none" ? "dummy" : x.EncryptMethod;
            singleproxy["password"] = x.Password;
            if(std::all_of(x.Password.begin(), x.Password.end(), ::isdigit) && !x.Password.empty())
                singleproxy["password"].SetTag("str");
            singleproxy["protocol"] = x.Protocol;
            singleproxy["obfs"] = x.OBFS;
            if(clashR)
            {
                singleproxy["protocolparam"] = x.ProtocolParam;
                singleproxy["obfsparam"] = x.OBFSParam;
            }
            else
            {
                singleproxy["protocol-param"] = x.ProtocolParam;
                singleproxy["obfs-param"] = x.OBFSParam;
            }
            break;
        case ProxyType::SOCKS5:
            singleproxy["type"] = "socks5";
            if(!x.Username.empty())
                singleproxy["username"] = x.Username;
            if(!x.Password.empty())
            {
                singleproxy["password"] = x.Password;
                if(std::all_of(x.Password.begin(), x.Password.end(), ::isdigit))
                    singleproxy["password"].SetTag("str");
            }
            if(!scv.is_undef())
                singleproxy["skip-cert-verify"] = scv.get();
            break;
        case ProxyType::HTTP:
        case ProxyType::HTTPS:
            singleproxy["type"] = "http";
            if(!x.Username.empty())
                singleproxy["username"] = x.Username;
            if(!x.Password.empty())
            {
                singleproxy["password"] = x.Password;
                if(std::all_of(x.Password.begin(), x.Password.end(), ::isdigit))
                    singleproxy["password"].SetTag("str");
            }
            singleproxy["tls"] = x.TLSSecure;
            if(!scv.is_undef())
                singleproxy["skip-cert-verify"] = scv.get();
            break;
        case ProxyType::Trojan:
            singleproxy["type"] = "trojan";
            singleproxy["password"] = x.Password;
            if(!x.Host.empty())
                singleproxy["sni"] = x.Host;
            if(std::all_of(x.Password.begin(), x.Password.end(), ::isdigit) && !x.Password.empty())
                singleproxy["password"].SetTag("str");
            if(!scv.is_undef())
                singleproxy["skip-cert-verify"] = scv.get();
            switch(hash_(x.TransferProtocol))
            {
            case "tcp"_hash:
                break;
            case "grpc"_hash:
                singleproxy["network"] = x.TransferProtocol;
                if(!x.Path.empty())
                    singleproxy["grpc-opts"]["grpc-service-name"] = x.Path;
                break;
            case "ws"_hash:
                singleproxy["network"] = x.TransferProtocol;
                singleproxy["ws-opts"]["path"] = x.Path;
                if(!x.Host.empty())
                    singleproxy["ws-opts"]["headers"]["Host"] = x.Host;
                break;
            }
            break;
        case ProxyType::Snell:
            singleproxy["type"] = "snell";
            singleproxy["psk"] = x.Password;
            if(x.SnellVersion != 0)
                singleproxy["version"] = x.SnellVersion;
            if(!x.OBFS.empty())
            {
                singleproxy["obfs-opts"]["mode"] = x.OBFS;
                if(!x.Host.empty())
                    singleproxy["obfs-opts"]["host"] = x.Host;
            }
            if(std::all_of(x.Password.begin(), x.Password.end(), ::isdigit) && !x.Password.empty())
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


    for(const ProxyGroupConfig &x : extra_proxy_group)
    {
        singlegroup.reset();
        eraseElements(filtered_nodelist);

        singlegroup["name"] = x.Name;
        singlegroup["type"] = x.TypeStr();

        switch(x.Type)
        {
        case ProxyGroupType::Select:
        case ProxyGroupType::Relay:
            break;
        case ProxyGroupType::LoadBalance:
            singlegroup["strategy"] = x.StrategyStr();
            [[fallthrough]];
        case ProxyGroupType::URLTest:
            if(!x.Lazy.is_undef())
                singlegroup["lazy"] = x.Lazy.get();
            [[fallthrough]];
        case ProxyGroupType::Fallback:
            singlegroup["url"] = x.Url;
            if(x.Interval > 0)
                singlegroup["interval"] = x.Interval;
            if(x.Tolerance > 0)
                singlegroup["tolerance"] = x.Tolerance;
            break;
        default:
            continue;
        }
        if(!x.DisableUdp.is_undef())
            singlegroup["disable-udp"] = x.DisableUdp.get();

        for(const auto& y : x.Proxies)
            groupGenerate(y, nodelist, filtered_nodelist, true, ext);

        if(!x.UsingProvider.empty())
            singlegroup["use"] = x.UsingProvider;
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
            if(original_groups[i]["name"].as<std::string>() == x.Name)
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

std::string proxyToClash(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, bool clashR, extra_settings &ext)
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

    proxyToClash(nodes, yamlnode, extra_proxy_group, clashR, ext);

    if(ext.nodelist)
        return YAML::Dump(yamlnode);

    /*
    if(ext.enable_rule_generator)
        rulesetToClash(yamlnode, ruleset_content_array, ext.overwrite_original_rules, ext.clash_new_field_name);

    return YAML::Dump(yamlnode);
    */
    if(!ext.enable_rule_generator)
        return YAML::Dump(yamlnode);

    if(!ext.managed_config_prefix.empty() || ext.clash_script)
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
    //rulesetToClash(yamlnode, ruleset_content_array, ext.overwrite_original_rules, ext.clash_new_field_name);
    //std::string output_content = YAML::Dump(yamlnode);

    return output_content;
}

std::string proxyToSurge(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, int surge_ver, extra_settings &ext)
{
    INIReader ini;
    std::string proxy;
    std::string output_nodelist;
    tribool udp, tfo, scv, tls13;
    std::vector<Proxy> nodelist;
    unsigned short local_port = 1080;

    string_array remarks_list, filtered_nodelist, args;

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

    for(Proxy &x : nodes)
    {
        std::string remark;
        if(ext.append_proxy_type)
        {
            std::string type = getProxyTypeName(x.Type);
            x.Remark = "[" + type + "] " + x.Remark;
        }

        processRemark(x.Remark, remark, remarks_list);

        std::string &hostname = x.Hostname, &username = x.Username, &password = x.Password, &method = x.EncryptMethod, &id = x.UserId, &transproto = x.TransferProtocol, &host = x.Host, &edge = x.Edge, &path = x.Path, &protocol = x.Protocol, &protoparam = x.ProtocolParam, &obfs = x.OBFS, &obfsparam = x.OBFSParam, &plugin = x.Plugin, &pluginopts = x.PluginOption;
        std::string port = std::to_string(x.Port);
        bool &tlssecure = x.TLSSecure;

        udp = ext.udp;
        tfo = ext.tfo;
        scv = ext.skip_cert_verify;
        tls13 = ext.tls13;
        udp.define(x.UDP);
        tfo.define(x.TCPFastOpen);
        scv.define(x.AllowInsecure);
        tls13.define(x.TLS13);

        proxy.clear();

        switch(x.Type)
        {
        case ProxyType::Shadowsocks:
            if(surge_ver >= 3 || surge_ver == -3)
            {
                proxy = "ss, " + hostname + ", " + port + ", encrypt-method=" + method + ", password=" + password;
            }
            else
            {
                proxy = "custom, "  + hostname + ", " + port + ", " + method + ", " + password + ", https://github.com/pobizhe/SSEncrypt/raw/master/SSEncrypt.module";
            }
            if(!plugin.empty())
            {
                switch(hash_(plugin))
                {
                case "simple-obfs"_hash:
                case "obfs-local"_hash:
                    if(!pluginopts.empty())
                        proxy += "," + replaceAllDistinct(pluginopts, ";", ",");
                    break;
                default:
                    continue;
                }
            }
            break;
        case ProxyType::VMess:
            if(surge_ver < 4 && surge_ver != -3)
                continue;
            proxy = "vmess, " + hostname + ", " + port + ", username=" + id + ", tls=" + (tlssecure ? "true" : "false") +  ", vmess-aead=" + (x.AlterId == 0 ? "true" : "false");
            if(tlssecure && !tls13.is_undef())
                proxy += ", tls13=" + std::string(tls13 ? "true" : "false");
            switch(hash_(transproto))
            {
            case "tcp"_hash:
                break;
            case "ws"_hash:
                if(host.empty())
                    proxy += ", ws=true, ws-path=" + path + ", sni=" + hostname;
                else
                    proxy += ", ws=true, ws-path=" + path + ", sni=" + hostname + ", ws-headers=Host:" + host;
                if(!edge.empty())
                    proxy += "|Edge:" + edge;
                break;
            default:
                continue;
            }
            if(!scv.is_undef())
                proxy += ", skip-cert-verify=" + scv.get_str();
            break;
        case ProxyType::ShadowsocksR:
            if(ext.surge_ssr_path.empty() || surge_ver < 2)
                continue;
            proxy = "external, exec=\"" + ext.surge_ssr_path + "\", args=\"";
            args = {"-l", std::to_string(local_port), "-s", hostname, "-p", port, "-m", method, "-k", password, "-o", obfs, "-O", protocol};
            if(!obfsparam.empty())
            {
                args.emplace_back("-g");
                args.emplace_back(std::move(obfsparam));
            }
            if(!protoparam.empty())
            {
                args.emplace_back("-G");
                args.emplace_back(std::move(protoparam));
            }
            proxy += join(args, "\", args=\"");
            proxy += "\", local-port=" + std::to_string(local_port);
            if(isIPv4(hostname) || isIPv6(hostname))
                proxy += ", addresses=" + hostname;
            else if(global.surgeResolveHostname)
                proxy += ", addresses=" + hostnameToIPAddr(hostname);
            local_port++;
            break;
        case ProxyType::SOCKS5:
            proxy = "socks5, " + hostname + ", " + port;
            if(!username.empty())
                proxy += ", username=" + username;
            if(!password.empty())
                proxy += ", password=" + password;
            if(!scv.is_undef())
                proxy += ", skip-cert-verify=" + scv.get_str();
            break;
        case ProxyType::HTTPS:
            if(surge_ver == -3)
            {
                proxy = "https, " + hostname + ", " + port + ", " + username + ", " + password;
                if(!scv.is_undef())
                    proxy += ", skip-cert-verify=" + scv.get_str();
                break;
            }
            [[fallthrough]];
        case ProxyType::HTTP:
            proxy = "http, " + hostname + ", " + port;
            if(!username.empty())
                proxy += ", username=" + username;
            if(!password.empty())
                proxy += ", password=" + password;
            proxy += std::string(", tls=") + (x.TLSSecure ? "true" : "false");
            if(!scv.is_undef())
                proxy += ", skip-cert-verify=" + scv.get_str();
            break;
        case ProxyType::Trojan:
            if(surge_ver < 4 && surge_ver != -3)
                continue;
            proxy = "trojan, " + hostname + ", " + port + ", password=" + password;
            if(x.SnellVersion != 0)
                proxy += ", version=" + std::to_string(x.SnellVersion);
            if(!host.empty())
                proxy += ", sni=" + host;
            if(!scv.is_undef())
                proxy += ", skip-cert-verify=" + scv.get_str();
            break;
        case ProxyType::Snell:
            proxy = "snell, " + hostname + ", " + port + ", psk=" + password;
            if(!obfs.empty())
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
    for(const ProxyGroupConfig &x : extra_proxy_group)
    {
        eraseElements(filtered_nodelist);
        proxy.clear();

        switch(x.Type)
        {
        case ProxyGroupType::Select:
        case ProxyGroupType::URLTest:
        case ProxyGroupType::Fallback:
            break;
        case ProxyGroupType::LoadBalance:
            if(surge_ver < 1 && surge_ver != -3)
                continue;
            break;
        case ProxyGroupType::SSID:
            proxy = x.TypeStr() + ",default=" + x.Proxies[0] + ",";
            proxy += join(x.Proxies.begin() + 1, x.Proxies.end(), ",");
            ini.Set("{NONAME}", x.Name + " = " + proxy); //insert order
            continue;
        default:
            continue;
        }

        for(const auto &y : x.Proxies)
            groupGenerate(y, nodelist, filtered_nodelist, true, ext);

        if(filtered_nodelist.empty())
            filtered_nodelist.emplace_back("DIRECT");

        if(filtered_nodelist.size() == 1)
        {
            proxy = toLower(filtered_nodelist[0]);
            switch(hash_(proxy))
            {
            case "direct"_hash:
            case "reject"_hash:
            case "reject-tinygif"_hash:
                ini.Set("Proxy", "{NONAME}", x.Name + " = " + proxy);
                continue;
            }
        }

        proxy = x.TypeStr() + ",";
        proxy += join(filtered_nodelist, ",");
        if(x.Type == ProxyGroupType::URLTest || x.Type == ProxyGroupType::Fallback || x.Type == ProxyGroupType::LoadBalance)
        {
            proxy += ",url=" + x.Url + ",interval=" + std::to_string(x.Interval);
            if(x.Tolerance > 0)
                proxy += ",tolerance=" + std::to_string(x.Tolerance);
            if(x.Timeout > 0)
                proxy += ",timeout=" + std::to_string(x.Timeout);
            if(!x.Persistent.is_undef())
                proxy += ",persistent=" + x.Persistent.get_str();
            if(!x.EvaluateBeforeUse.is_undef())
                proxy += ",evaluate-before-use=" + x.EvaluateBeforeUse.get_str();
        }

        ini.Set("{NONAME}", x.Name + " = " + proxy); //insert order
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, surge_ver, ext.overwrite_original_rules, ext.managed_config_prefix);

    return ini.ToString();
}

std::string proxyToSingle(std::vector<Proxy> &nodes, int types, extra_settings &ext)
{
    /// types: SS=1 SSR=2 VMess=4 Trojan=8
    std::string remark, hostname, port, password, method;
    std::string plugin, pluginopts;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string id, aid, transproto, faketype, host, path, quicsecure, quicsecret;
    std::string proxyStr, allLinks;
    bool ss = GETBIT(types, 1), ssr = GETBIT(types, 2), vmess = GETBIT(types, 3), trojan = GETBIT(types, 4);

    for(Proxy &x : nodes)
    {
        remark = x.Remark;
        std::string &hostname = x.Hostname, &password = x.Password, &method = x.EncryptMethod, &plugin = x.Plugin, &pluginopts = x.PluginOption, &protocol = x.Protocol, &protoparam = x.ProtocolParam, &obfs = x.OBFS, &obfsparam = x.OBFSParam, &id = x.UserId, &transproto = x.TransferProtocol, &host = x.Host, &path = x.Path, &faketype = x.FakeType;
        bool &tlssecure = x.TLSSecure;
        std::string port = std::to_string(x.Port);
        std::string aid = std::to_string(x.AlterId);

        switch(x.Type)
        {
        case ProxyType::Shadowsocks:
            if(ss)
            {
                proxyStr = "ss://" + urlSafeBase64Encode(method + ":" + password) + "@" + hostname + ":" + port;
                if(!plugin.empty() && !pluginopts.empty())
                {
                    proxyStr += "/?plugin=" + urlEncode(plugin + ";" + pluginopts);
                }
                proxyStr += "#" + urlEncode(remark);
            }
            else if(ssr)
            {
                if(std::find(ssr_ciphers.begin(), ssr_ciphers.end(), method) != ssr_ciphers.end() && plugin.empty())
                    proxyStr = "ssr://" + urlSafeBase64Encode(hostname + ":" + port + ":origin:" + method + ":plain:" + urlSafeBase64Encode(password) \
                               + "/?group=" + urlSafeBase64Encode(x.Group) + "&remarks=" + urlSafeBase64Encode(remark));
            }
            else
                continue;
            break;
        case ProxyType::ShadowsocksR:
            if(ssr)
            {
                proxyStr = "ssr://" + urlSafeBase64Encode(hostname + ":" + port + ":" + protocol + ":" + method + ":" + obfs + ":" + urlSafeBase64Encode(password) \
                           + "/?group=" + urlSafeBase64Encode(x.Group) + "&remarks=" + urlSafeBase64Encode(remark) \
                           + "&obfsparam=" + urlSafeBase64Encode(obfsparam) + "&protoparam=" + urlSafeBase64Encode(protoparam));
            }
            else if(ss)
            {
                if(std::find(ss_ciphers.begin(), ss_ciphers.end(), method) != ss_ciphers.end() && protocol == "origin" && obfs == "plain")
                    proxyStr = "ss://" + urlSafeBase64Encode(method + ":" + password) + "@" + hostname + ":" + port + "#" + urlEncode(remark);
            }
            else
                continue;
            break;
        case ProxyType::VMess:
            if(!vmess)
                continue;
            proxyStr = "vmess://" + base64Encode(vmessLinkConstruct(remark, hostname, port, faketype, id, aid, transproto, path, host, tlssecure ? "tls" : ""));
            break;
        case ProxyType::Trojan:
            if(!trojan)
                continue;
            proxyStr = "trojan://" + password + "@" + hostname + ":" + port + "?allowInsecure=" + (x.AllowInsecure.get() ? "1" : "0");
            if(!host.empty())
                proxyStr += "&sni=" + host;
            if(transproto == "ws")
            {
                proxyStr += "&ws=1";
                if(!path.empty())
                    proxyStr += "&wspath=" + urlEncode(path);
            }
            proxyStr += "#" + urlEncode(remark);
            break;
        default:
            continue;
        }
        allLinks += proxyStr + "\n";
    }

    if(ext.nodelist)
        return allLinks;
    else
        return base64Encode(allLinks);
}

std::string proxyToSSSub(std::string base_conf, std::vector<Proxy> &nodes, extra_settings &ext)
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

    base_conf = trimWhitespace(base_conf);
    if(base_conf.empty())
        base_conf = "{}";
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
    for(Proxy &x : nodes)
    {
        remark = x.Remark;
        hostname = x.Hostname;
        std::string &password = x.Password;
        std::string &method = x.EncryptMethod;
        std::string &plugin = x.Plugin;
        std::string &pluginopts = x.PluginOption;
        std::string &protocol = x.Protocol;
        std::string &obfs = x.OBFS;

        switch(x.Type)
        {
        case ProxyType::Shadowsocks:
            if(plugin == "simple-obfs")
                plugin = "obfs-local";
            break;
        case ProxyType::ShadowsocksR:
            if(std::find(ss_ciphers.begin(), ss_ciphers.end(), method) == ss_ciphers.end() || protocol != "origin" || obfs != "plain")
                continue;
            break;
        default:
            continue;
        }
        jsondata["remarks"].SetString(rapidjson::StringRef(remark.c_str(), remark.size()));
        jsondata["server"].SetString(rapidjson::StringRef(hostname.c_str(), hostname.size()));
        jsondata["server_port"] = x.Port;
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

std::string proxyToQuan(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext)
{
    INIReader ini;
    ini.store_any_line = true;
    if(!ext.nodelist && ini.Parse(base_conf) != 0)
    {
        writeLog(0, "Quantumult base loader failed with error: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return std::string();
    }

    proxyToQuan(nodes, ini, ruleset_content_array, extra_proxy_group, ext);

    if(ext.nodelist)
    {
        string_array allnodes;
        std::string allLinks;
        ini.GetAll("SERVER", "{NONAME}", allnodes);
        if(!allnodes.empty())
            allLinks = join(allnodes, "\n");
        return base64Encode(allLinks);
    }
    return ini.ToString();
}

void proxyToQuan(std::vector<Proxy> &nodes, INIReader &ini, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext)
{
    std::string type, proxyStr;
    tribool scv;
    std::vector<Proxy> nodelist;
    string_array remarks_list;

    ini.SetCurrentSection("SERVER");
    ini.EraseSection();
    for(Proxy &x : nodes)
    {
        std::string remark = x.Remark;

        if(ext.append_proxy_type)
        {
            std::string type = getProxyTypeName(x.Type);
            x.Remark = "[" + type + "] " + x.Remark;
        }

        processRemark(x.Remark, remark, remarks_list);

        std::string &hostname = x.Hostname, &method = x.EncryptMethod, &password = x.Password, &id = x.UserId, &transproto = x.TransferProtocol, &host = x.Host, &path = x.Path, &edge = x.Edge, &protocol = x.Protocol, &protoparam = x.ProtocolParam, &obfs = x.OBFS, &obfsparam = x.OBFSParam, &plugin = x.Plugin, &pluginopts = x.PluginOption, &username = x.Username;
        std::string port = std::to_string(x.Port);
        bool &tlssecure = x.TLSSecure;

        switch(x.Type)
        {
        case ProxyType::VMess:
            scv = ext.skip_cert_verify;
            scv.define(x.AllowInsecure);

            if(method == "auto")
                method = "chacha20-ietf-poly1305";
            proxyStr = remark + " = vmess, " + hostname + ", " + port + ", " + method + ", \"" + id + "\", group=" + x.Group;
            if(tlssecure)
            {
                proxyStr += ", over-tls=true, tls-host=" + host;
                if(!scv.is_undef())
                    proxyStr += ", certificate=" + std::string(scv.get() ? "0" : "1");
            }
            if(transproto == "ws")
            {
                proxyStr += ", obfs=ws, obfs-path=\"" + path + "\", obfs-header=\"Host: " + host;
                if(!edge.empty())
                    proxyStr += "[Rr][Nn]Edge: " + edge;
                proxyStr += "\"";
            }

            if(ext.nodelist)
                proxyStr = "vmess://" + urlSafeBase64Encode(proxyStr);
            break;
        case ProxyType::ShadowsocksR:
            if(ext.nodelist)
            {
                proxyStr = "ssr://" + urlSafeBase64Encode(hostname + ":" + port + ":" + protocol + ":" + method + ":" + obfs + ":" + urlSafeBase64Encode(password) \
                           + "/?group=" + urlSafeBase64Encode(x.Group) + "&remarks=" + urlSafeBase64Encode(remark) \
                           + "&obfsparam=" + urlSafeBase64Encode(obfsparam) + "&protoparam=" + urlSafeBase64Encode(protoparam));
            }
            else
            {
                proxyStr = remark + " = shadowsocksr, " + hostname + ", " + port + ", " + method + ", \"" + password + "\", group=" + x.Group + ", protocol=" + protocol + ", obfs=" + obfs;
                if(!protoparam.empty())
                    proxyStr += ", protocol_param=" + protoparam;
                if(!obfsparam.empty())
                    proxyStr += ", obfs_param=" + obfsparam;
            }
            break;
        case ProxyType::Shadowsocks:
            if(ext.nodelist)
            {
                proxyStr = "ss://" + urlSafeBase64Encode(method + ":" + password) + "@" + hostname + ":" + port;
                if(!plugin.empty() && !pluginopts.empty())
                {
                    proxyStr += "/?plugin=" + urlEncode(plugin + ";" + pluginopts);
                }
                proxyStr += "&group=" + urlSafeBase64Encode(x.Group) + "#" + urlEncode(remark);
            }
            else
            {
                proxyStr = remark + " = shadowsocks, " + hostname + ", " + port + ", " + method + ", \"" + password + "\", group=" + x.Group;
                if(plugin == "obfs-local" && !pluginopts.empty())
                {
                    proxyStr += ", " + replaceAllDistinct(pluginopts, ";", ", ");
                }
            }
            break;
        case ProxyType::HTTP:
        case ProxyType::HTTPS:
            proxyStr = remark + " = http, upstream-proxy-address=" + hostname + ", upstream-proxy-port=" + port + ", group=" + x.Group;
            if(!username.empty() && !password.empty())
                proxyStr += ", upstream-proxy-auth=true, upstream-proxy-username=" + username + ", upstream-proxy-password=" + password;
            else
                proxyStr += ", upstream-proxy-auth=false";

            if(tlssecure)
            {
                proxyStr += ", over-tls=true";
                if(!host.empty())
                    proxyStr += ", tls-host=" + host;
                if(!scv.is_undef())
                    proxyStr += ", certificate=" + std::string(scv.get() ? "0" : "1");
            }

            if(ext.nodelist)
                proxyStr = "http://" + urlSafeBase64Encode(proxyStr);
            break;
        case ProxyType::SOCKS5:
            proxyStr = remark + " = socks, upstream-proxy-address=" + hostname + ", upstream-proxy-port=" + port + ", group=" + x.Group;
            if(!username.empty() && !password.empty())
                proxyStr += ", upstream-proxy-auth=true, upstream-proxy-username=" + username + ", upstream-proxy-password=" + password;
            else
                proxyStr += ", upstream-proxy-auth=false";

            if(tlssecure)
            {
                proxyStr += ", over-tls=true";
                if(!host.empty())
                    proxyStr += ", tls-host=" + host;
                if(!scv.is_undef())
                    proxyStr += ", certificate=" + std::string(scv.get() ? "0" : "1");
            }

            if(ext.nodelist)
                proxyStr = "socks://" + urlSafeBase64Encode(proxyStr);
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
    for(const ProxyGroupConfig &x : extra_proxy_group)
    {
        eraseElements(filtered_nodelist);

        switch(x.Type)
        {
        case ProxyGroupType::Select:
        case ProxyGroupType::Fallback:
            type = "static";
            break;
        case ProxyGroupType::URLTest:
            type = "auto";
            break;
        case ProxyGroupType::LoadBalance:
            type = "balance, round-robin";
            break;
        case ProxyGroupType::SSID:
            {
                singlegroup = x.Name + " : wifi = " + x.Proxies[0];
                std::string content, celluar, celluar_matcher = R"(^(.*?),?celluar\s?=\s?(.*?)(,.*)$)", rem_a, rem_b;
                for(auto iter = x.Proxies.begin() + 1; iter != x.Proxies.end(); iter++)
                {
                    if(regGetMatch(*iter, celluar_matcher, 4, 0, &rem_a, &celluar, &rem_b))
                    {
                        content += *iter + "\n";
                        continue;
                    }
                    content += rem_a + rem_b + "\n";
                }
                if(!celluar.empty())
                    singlegroup += ", celluar = " + celluar;
                singlegroup += "\n" + replaceAllDistinct(trimOf(content, ','), ",", "\n");
                ini.Set("{NONAME}", base64Encode(singlegroup)); //insert order
            }
            continue;
        default:
            continue;
        }

        for(const auto &y : x.Proxies)
            groupGenerate(y, nodelist, filtered_nodelist, true, ext);

        if(filtered_nodelist.empty())
            filtered_nodelist.emplace_back("direct");

        if(filtered_nodelist.size() < 2) // force groups with 1 node to be static
            type = "static";

        proxies = join(filtered_nodelist, "\n");

        singlegroup = x.Name + " : " + type;
        if(type == "static")
            singlegroup += ", " + filtered_nodelist[0];
        singlegroup += "\n" + proxies + "\n";
        ini.Set("{NONAME}", base64Encode(singlegroup));
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, -2, ext.overwrite_original_rules, std::string());
}

std::string proxyToQuanX(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext)
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

    proxyToQuanX(nodes, ini, ruleset_content_array, extra_proxy_group, ext);

    if(ext.nodelist)
    {
        string_array allnodes;
        std::string allLinks;
        ini.GetAll("server_local", "{NONAME}", allnodes);
        if(!allnodes.empty())
            allLinks = join(allnodes, "\n");
        return allLinks;
    }
    return ini.ToString();
}

void proxyToQuanX(std::vector<Proxy> &nodes, INIReader &ini, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext)
{
    std::string type;
    std::string remark, hostname, port, method;
    std::string password, plugin, pluginopts;
    std::string id, transproto, host, path;
    std::string protocol, protoparam, obfs, obfsparam;
    std::string proxyStr;
    tribool udp, tfo, scv, tls13;
    std::vector<Proxy> nodelist;
    string_array remarks_list;

    ini.SetCurrentSection("server_local");
    ini.EraseSection();
    for(Proxy &x : nodes)
    {
        if(ext.append_proxy_type)
            x.Remark = "[" + type + "] " + x.Remark;

        processRemark(x.Remark, remark, remarks_list);

        std::string &hostname = x.Hostname, &method = x.EncryptMethod, &id = x.UserId, &transproto = x.TransferProtocol, &host = x.Host, &path = x.Path, &password = x.Password, &plugin = x.Plugin, &pluginopts = x.PluginOption, &protocol = x.Protocol, &protoparam = x.ProtocolParam, &obfs = x.OBFS, &obfsparam = x.OBFSParam, &username = x.Username;
        std::string port = std::to_string(x.Port);
        bool &tlssecure = x.TLSSecure;

        udp = ext.udp;
        tfo = ext.tfo;
        scv = ext.skip_cert_verify;
        tls13 = ext.tls13;
        udp.define(x.UDP);
        tfo.define(x.TCPFastOpen);
        scv.define(x.AllowInsecure);
        tls13.define(x.TLS13);

        switch(x.Type)
        {
        case ProxyType::VMess:
            if(method == "auto")
                method = "chacha20-ietf-poly1305";
            proxyStr = "vmess = " + hostname + ":" + port + ", method=" + method + ", password=" + id + ", aead=" + (x.AlterId == 0 ? "true" : "false");
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
        case ProxyType::Shadowsocks:
            proxyStr = "shadowsocks = " + hostname + ":" + port + ", method=" + method + ", password=" + password;
            if(!plugin.empty())
            {
                switch(hash_(plugin))
                {
                    case "simple-obfs"_hash:
                    case "obfs-local"_hash:
                        if(!pluginopts.empty())
                            proxyStr += ", " + replaceAllDistinct(pluginopts, ";", ", ");
                        break;
                    case "v2ray-plugin"_hash:
                        pluginopts = replaceAllDistinct(pluginopts, ";", "&");
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
                        if(!host.empty())
                            proxyStr += ", obfs-host=" + host;
                        if(!path.empty())
                            proxyStr += ", obfs-uri=" + path;
                        break;
                    default: continue;
                }
            }

            break;
        case ProxyType::ShadowsocksR:
            proxyStr = "shadowsocks = " + hostname + ":" + port + ", method=" + method + ", password=" + password + ", ssr-protocol=" + protocol;
            if(!protoparam.empty())
                proxyStr += ", ssr-protocol-param=" + protoparam;
            proxyStr += ", obfs=" + obfs;
            if(!obfsparam.empty())
                proxyStr += ", obfs-host=" + obfsparam;
            break;
        case ProxyType::HTTP:
        case ProxyType::HTTPS:
            proxyStr = "http = " + hostname + ":" + port + ", username=" + (username.empty() ? "none" : username) + ", password=" + (password.empty() ? "none" : password);
            if(tlssecure)
            {
                proxyStr += ", over-tls=true";
                if(!tls13.is_undef())
                    proxyStr += ", tls13=" + std::string(tls13 ? "true" : "false");
            }
            else
            {
                proxyStr += ", over-tls=false";
            }
            break;
        case ProxyType::Trojan:
            proxyStr = "trojan = " + hostname + ":" + port + ", password=" + password;
            if(tlssecure)
            {
                proxyStr += ", over-tls=true, tls-host=" + host;
                if(!tls13.is_undef())
                    proxyStr += ", tls13=" + std::string(tls13 ? "true" : "false");
            }
            else
            {
                proxyStr += ", over-tls=false";
            }
            break;
        case ProxyType::SOCKS5:
            proxyStr = "socks5 = " + hostname + ":" + port;
            if(!username.empty() && !password.empty())
            {
                proxyStr += ", username=" + username + ", password=" + password;
                if(tlssecure)
                {
                    proxyStr += ", over-tls=true, tls-host=" + host;
                    if(!tls13.is_undef())
                        proxyStr += ", tls13=" + std::string(tls13 ? "true" : "false");
                }
                else
                {
                    proxyStr += ", over-tls=false";
                }
            }
            break;
        default:
            continue;
        }
        if(!tfo.is_undef())
            proxyStr += ", fast-open=" + tfo.get_str();
        if(!udp.is_undef())
            proxyStr += ", udp-relay=" + udp.get_str();
        if(tlssecure && !scv.is_undef() && (x.Type != ProxyType::Shadowsocks && x.Type != ProxyType::ShadowsocksR))
            proxyStr += ", tls-verification=" + scv.reverse().get_str();
        proxyStr += ", tag=" + remark;

        ini.Set("{NONAME}", proxyStr);
        remarks_list.emplace_back(std::move(remark));
        nodelist.emplace_back(x);
    }

    if(ext.nodelist)
        return;

    string_multimap original_groups;
    ini.SetCurrentSection("policy");
    ini.GetItems(original_groups);
    ini.EraseSection();

    for(const ProxyGroupConfig &x : extra_proxy_group)
    {
        string_array filtered_nodelist;

        switch(x.Type)
        {
        case ProxyGroupType::Select:
            type = "static";
            break;
        case ProxyGroupType::URLTest:
            type = "url-latency-benchmark";
            break;
        case ProxyGroupType::Fallback:
            type = "available";
            break;
        case ProxyGroupType::LoadBalance:
            type = "round-robin";
            break;
        case ProxyGroupType::SSID:
            type = "ssid";
            for(auto iter = x.Proxies.begin(); iter != x.Proxies.end(); iter++)
                filtered_nodelist.emplace_back(replaceAllDistinct(*iter, "=", ":"));
            break;
        default:
            continue;
        }

        if(x.Type != ProxyGroupType::SSID)
        {
            for(const auto &y : x.Proxies)
                groupGenerate(y, nodelist, filtered_nodelist, true, ext);

            if(filtered_nodelist.empty())
                filtered_nodelist.emplace_back("direct");

            if(filtered_nodelist.size() < 2) // force groups with 1 node to be static
                type = "static";
        }

        auto iter = std::find_if(original_groups.begin(), original_groups.end(), [&](const string_multimap::value_type &n)
        {
            std::string groupdata = n.second;
            std::string::size_type cpos = groupdata.find(",");
            if(cpos != groupdata.npos)
                return trim(groupdata.substr(0, cpos)) == x.Name;
            else
                return false;
        });
        if(iter != original_groups.end())
        {
            string_array vArray = split(iter->second, ",");
            if(vArray.size() > 1)
            {
                if(trim(vArray[vArray.size() - 1]).find("img-url") == 0)
                    filtered_nodelist.emplace_back(trim(vArray[vArray.size() - 1]));
            }
        }

        std::string proxies = join(filtered_nodelist, ", ");

        std::string singlegroup = type + "=" + x.Name + ", " + proxies;
        ini.Set("{NONAME}", singlegroup);
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, -1, ext.overwrite_original_rules, ext.managed_config_prefix);
}

std::string proxyToSSD(std::vector<Proxy> &nodes, std::string &group, std::string &userinfo, extra_settings &ext)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    size_t index = 0;

    if(group.empty())
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
    if(!userinfo.empty())
    {
        std::string data = replaceAllDistinct(userinfo, "; ", "&");
        std::string upload = getUrlArg(data, "upload"), download = getUrlArg(data, "download"), total = getUrlArg(data, "total"), expiry = getUrlArg(data, "expire");
        double used = (to_number(upload, 0.0) + to_number(download, 0.0)) / std::pow(1024, 3) * 1.0, tot = to_number(total, 0.0) / std::pow(1024, 3) * 1.0;
        writer.Key("traffic_used");
        writer.Double(used);
        writer.Key("traffic_total");
        writer.Double(tot);
        if(!expiry.empty())
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

    for(Proxy &x : nodes)
    {
        std::string &hostname = x.Hostname, &password = x.Password, &method = x.EncryptMethod, &plugin = x.Plugin, &pluginopts = x.PluginOption, &protocol = x.Protocol, &obfs = x.OBFS;

        switch(x.Type)
        {
        case ProxyType::Shadowsocks:
            if(plugin == "obfs-local")
                plugin = "simple-obfs";
            writer.StartObject();
            writer.Key("server");
            writer.String(hostname.data());
            writer.Key("port");
            writer.Int(x.Port);
            writer.Key("encryption");
            writer.String(method.data());
            writer.Key("password");
            writer.String(password.data());
            writer.Key("plugin");
            writer.String(plugin.data());
            writer.Key("plugin_options");
            writer.String(pluginopts.data());
            writer.Key("remarks");
            writer.String(x.Remark.data());
            writer.Key("id");
            writer.Int(index);
            writer.EndObject();
            break;
        case ProxyType::ShadowsocksR:
            if(std::count(ss_ciphers.begin(), ss_ciphers.end(), method) > 0 && protocol == "origin" && obfs == "plain")
            {
                writer.StartObject();
                writer.Key("server");
                writer.String(hostname.data());
                writer.Key("port");
                writer.Int(x.Port);
                writer.Key("encryption");
                writer.String(method.data());
                writer.Key("password");
                writer.String(password.data());
                writer.Key("remarks");
                writer.String(x.Remark.data());
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
    return "ssd://" + base64Encode(sb.GetString());
}

std::string proxyToMellow(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext)
{
    INIReader ini;
    ini.store_any_line = true;
    if(ini.Parse(base_conf) != 0)
    {
        writeLog(0, "Mellow base loader failed with error: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return std::string();
    }

    proxyToMellow(nodes, ini, ruleset_content_array, extra_proxy_group, ext);

    return ini.ToString();
}

void proxyToMellow(std::vector<Proxy> &nodes, INIReader &ini, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext)
{
    std::string proxy;
    std::string type, remark, hostname, port, username, password, method;
    std::string plugin, pluginopts;
    std::string id, aid, transproto, faketype, host, path, quicsecure, quicsecret, tlssecure;
    std::string url;
    tribool tfo, scv;
    std::vector<Proxy> nodelist;
    string_array vArray, remarks_list, filtered_nodelist;

    ini.SetCurrentSection("Endpoint");

    for(Proxy &x : nodes)
    {
        if(ext.append_proxy_type)
        {
            std::string type = getProxyTypeName(x.Type);
            x.Remark = "[" + type + "] " + x.Remark;
        }

        processRemark(x.Remark, remark, remarks_list);

        std::string &hostname = x.Hostname;
        port = std::to_string(x.Port);

        tfo = ext.tfo;
        scv = ext.skip_cert_verify;
        tfo.define(x.TCPFastOpen);
        scv.define(x.AllowInsecure);

        switch(x.Type)
        {
        case ProxyType::Shadowsocks:
            if(!x.Plugin.empty())
                continue;
            proxy = remark + ", ss, ss://" + urlSafeBase64Encode(method + ":" + password) + "@" + hostname + ":" + port;
            break;
        case ProxyType::VMess:
            proxy = remark + ", vmess1, vmess1://" + id + "@" + hostname + ":" + port;
            if(!path.empty())
                proxy += path;
            proxy += "?network=" + transproto;
            switch(hash_(transproto))
            {
            case "ws"_hash:
                proxy += "&ws.host=" + urlEncode(host);
                break;
            case "http"_hash:
                if(!host.empty())
                    proxy += "&http.host=" + urlEncode(host);
                break;
            case "quic"_hash:
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
                    proxy += "&tls.servername=" + urlEncode(host);
            }
            if(!scv.is_undef())
                proxy += "&tls.allowinsecure=" + scv.get_str();
            if(!tfo.is_undef())
                proxy += "&sockopt.tcpfastopen=" + tfo.get_str();
            break;
        case ProxyType::SOCKS5:
            proxy = remark + ", builtin, socks, address=" + hostname + ", port=" + port + ", user=" + username + ", pass=" + password;
            break;
        case ProxyType::HTTP:
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

    for(const ProxyGroupConfig &x : extra_proxy_group)
    {
        eraseElements(filtered_nodelist);
        url.clear();
        proxy.clear();

        switch(x.Type)
        {
        case ProxyGroupType::Select:
        case ProxyGroupType::URLTest:
        case ProxyGroupType::Fallback:
        case ProxyGroupType::LoadBalance:
            break;
        default:
            continue;
        }

        for(const auto &y : x.Proxies)
            groupGenerate(y, nodelist, filtered_nodelist, false, ext);

        if(filtered_nodelist.empty())
        {
            if(remarks_list.empty())
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

        proxy = x.Name + ", ";
        /*
        for(std::string &y : filtered_nodelist)
            proxy += y + ":";
        proxy = proxy.substr(0, proxy.size() - 1);
        */
        proxy += join(filtered_nodelist, ":");
        proxy += ", latency, interval=300, timeout=6"; //use hard-coded values for now

        ini.Set("{NONAME}", proxy); //insert order
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, 0, ext.overwrite_original_rules, std::string());
}

std::string proxyToLoon(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext)
{
    rapidjson::Document json;
    INIReader ini;
    std::string proxy;
    std::string output_nodelist;
    tribool scv;
    std::vector<Proxy> nodelist;

    string_array vArray, remarks_list, filtered_nodelist;

    ini.store_any_line = true;
    if(ini.Parse(base_conf) != INIREADER_EXCEPTION_NONE && !ext.nodelist)
    {
        writeLog(0, "Loon base loader failed with error: " + ini.GetLastError(), LOG_LEVEL_ERROR);
        return std::string();
    }


    ini.SetCurrentSection("Proxy");
    ini.EraseSection();

    for(Proxy &x : nodes)
    {
        if(ext.append_proxy_type)
        {
            std::string type = getProxyTypeName(x.Type);
            x.Remark = "[" + type + "] " + x.Remark;
        }
        std::string remark = x.Remark;
        processRemark(x.Remark, remark, remarks_list);

        std::string &hostname = x.Hostname, &username = x.Username, &password = x.Password, &method = x.EncryptMethod, &plugin = x.Plugin, &pluginopts = x.PluginOption, &id = x.UserId, &transproto = x.TransferProtocol, &host = x.Host, &path = x.Path, &protocol = x.Protocol, &protoparam = x.ProtocolParam, &obfs = x.OBFS, &obfsparam = x.OBFSParam;
        std::string port = std::to_string(x.Port), aid = std::to_string(x.AlterId);
        bool &tlssecure = x.TLSSecure;

        tribool scv = ext.skip_cert_verify;
        scv.define(x.AllowInsecure);

        proxy.clear();

        switch(x.Type)
        {
        case ProxyType::Shadowsocks:
            proxy = "Shadowsocks," + hostname + "," + port + "," + method + ",\"" + password + "\"";
            if(plugin == "simple-obfs" || plugin == "obfs-local")
            {
                if(!pluginopts.empty())
                    proxy += "," + replaceAllDistinct(replaceAllDistinct(pluginopts, ";obfs-host=", ","), "obfs=", "");
            }
            else if(!plugin.empty())
                continue;
            break;
        case ProxyType::VMess:
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
        case ProxyType::ShadowsocksR:
            proxy = "ShadowsocksR," + hostname + "," + port + "," + method + ",\"" + password + "\"," + protocol + ",{" + protoparam + "}," + obfs + ",{" + obfsparam + "}";
            break;
        /*
        case ProxyType::SOCKS5:
            proxy = "socks5, " + hostname + ", " + port + ", " + username + ", " + password;
            if(ext.skip_cert_verify)
                proxy += ", skip-cert-verify:1";
            break;
        */
        case ProxyType::HTTP:
            proxy = "http," + hostname + "," + port + "," + username + "," + password;
            break;
        case ProxyType::Trojan:
            proxy = "trojan," + hostname + "," + port + "," + password;
            if(!host.empty())
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
    for(const ProxyGroupConfig &x : extra_proxy_group)
    {
        eraseElements(filtered_nodelist);
        proxy.clear();

        switch(x.Type)
        {
        case ProxyGroupType::Select:
        case ProxyGroupType::URLTest:
        case ProxyGroupType::Fallback:
            break;
        case ProxyGroupType::SSID:
            if(x.Proxies.size() < 2)
                continue;
            proxy = x.TypeStr() + ",default=" + x.Proxies[0] + ",";
            proxy += join(x.Proxies.begin() + 1, x.Proxies.end(), ",");
            ini.Set("{NONAME}", x.Name + " = " + proxy); //insert order
            continue;
        default:
            continue;
        }

        for(const auto &y : x.Proxies)
            groupGenerate(y, nodelist, filtered_nodelist, true, ext);

        if(filtered_nodelist.empty())
            filtered_nodelist.emplace_back("DIRECT");

        proxy = x.TypeStr() + ",";
        /*
        for(std::string &y : filtered_nodelist)
            proxy += "," + y;
        */
        proxy += join(filtered_nodelist, ",");
        if(x.Type == ProxyGroupType::URLTest || x.Type == ProxyGroupType::Fallback)
            proxy += ",url=" + x.Url + ",interval=" + std::to_string(x.Interval);

        ini.Set("{NONAME}", x.Name + " = " + proxy); //insert order
    }

    if(ext.enable_rule_generator)
        rulesetToSurge(ini, ruleset_content_array, -4, ext.overwrite_original_rules, ext.managed_config_prefix);

    return ini.ToString();
}
