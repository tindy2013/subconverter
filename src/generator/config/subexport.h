#ifndef SUBEXPORT_H_INCLUDED
#define SUBEXPORT_H_INCLUDED

#include <string>

#ifndef NO_JS_RUNTIME
#include <quickjspp.hpp>
#endif // NO_JS_RUNTIME

#include "config/proxygroup.h"
#include "config/regmatch.h"
#include "parser/config/proxy.h"
#include "utils/ini_reader/ini_reader.h"
#include "utils/string.h"
#include "utils/yamlcpp_extra.h"
#include "ruleconvert.h"

struct extra_settings
{
    bool enable_rule_generator = true;
    bool overwrite_original_rules = true;
    RegexMatchConfigs rename_array;
    RegexMatchConfigs emoji_array;
    bool add_emoji = false;
    bool remove_emoji = false;
    bool append_proxy_type = false;
    bool nodelist = false;
    bool sort_flag = false;
    bool filter_deprecated = false;
    bool clash_new_field_name = false;
    bool clash_script = false;
    std::string surge_ssr_path;
    std::string managed_config_prefix;
    std::string quanx_dev_id;
    tribool udp = tribool();
    tribool tfo = tribool();
    tribool skip_cert_verify = tribool();
    tribool tls13 = tribool();
    bool clash_classical_ruleset = false;
    std::string sort_script;
    std::string clash_proxies_style = "flow";
    std::string clash_proxy_groups_style = "flow";
    bool authorized = false;

    extra_settings() = default;
    extra_settings(const extra_settings&) = delete;
    extra_settings(extra_settings&&) = delete;

#ifndef NO_JS_RUNTIME
    qjs::Runtime *js_runtime = nullptr;
    qjs::Context *js_context = nullptr;

    ~extra_settings()
    {
        delete js_context;
        delete js_runtime;
    }
#endif // NO_JS_RUNTIME
};

std::string proxyToClash(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, bool clashR, extra_settings &ext);
void proxyToClash(std::vector<Proxy> &nodes, YAML::Node &yamlnode, const ProxyGroupConfigs &extra_proxy_group, bool clashR, extra_settings &ext);
std::string proxyToSurge(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, int surge_ver, extra_settings &ext);
std::string proxyToMellow(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext);
void proxyToMellow(std::vector<Proxy> &nodes, INIReader &ini, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext);
std::string proxyToLoon(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext);
std::string proxyToSSSub(std::string base_conf, std::vector<Proxy> &nodes, extra_settings &ext);
std::string proxyToSingle(std::vector<Proxy> &nodes, int types, extra_settings &ext);
std::string proxyToQuanX(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext);
void proxyToQuanX(std::vector<Proxy> &nodes, INIReader &ini, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext);
std::string proxyToQuan(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext);
void proxyToQuan(std::vector<Proxy> &nodes, INIReader &ini, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext);
std::string proxyToSSD(std::vector<Proxy> &nodes, std::string &group, std::string &userinfo, extra_settings &ext);
std::string proxyToSingBox(std::vector<Proxy> &nodes, const std::string &base_conf, std::vector<RulesetContent> &ruleset_content_array, const ProxyGroupConfigs &extra_proxy_group, extra_settings &ext);

#endif // SUBEXPORT_H_INCLUDED
