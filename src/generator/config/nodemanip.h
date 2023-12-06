#ifndef NODEMANIP_H_INCLUDED
#define NODEMANIP_H_INCLUDED

#include <string>
#include <vector>
#include <limits.h>

#ifndef NO_JS_RUNTIME
#include <quickjspp.hpp>
#endif // NO_JS_RUNTIME

#include "config/regmatch.h"
#include "parser/config/proxy.h"
#include "utils/map_extra.h"
#include "utils/string.h"

struct parse_settings
{
    std::string *proxy = nullptr;
    string_array *exclude_remarks = nullptr;
    string_array *include_remarks = nullptr;
    RegexMatchConfigs *stream_rules = nullptr;
    RegexMatchConfigs *time_rules = nullptr;
    std::string *sub_info = nullptr;
    bool authorized = false;
    string_icase_map *request_header = nullptr;
#ifndef NO_JS_RUNTIME
    qjs::Runtime *js_runtime = nullptr;
    qjs::Context *js_context = nullptr;
#endif // NO_JS_RUNTIME
};

int addNodes(std::string link, std::vector<Proxy> &allNodes, int groupID, parse_settings &parse_set);
void filterNodes(std::vector<Proxy> &nodes, string_array &exclude_remarks, string_array &include_remarks, int groupID);
bool applyMatcher(const std::string &rule, std::string &real_rule, const Proxy &node);
void preprocessNodes(std::vector<Proxy> &nodes, extra_settings &ext);

#endif // NODEMANIP_H_INCLUDED
