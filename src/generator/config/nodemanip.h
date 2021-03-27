#ifndef NODEMANIP_H_INCLUDED
#define NODEMANIP_H_INCLUDED

#include <string>
#include <vector>
#include <limits.h>
#include <quickjspp.hpp>

#include "../../parser/config/proxy.h"
#include "../../utils/map_extra.h"
#include "../../utils/string.h"

struct parse_settings
{
    std::string *proxy = nullptr;
    string_array *exclude_remarks = nullptr;
    string_array *include_remarks = nullptr;
    string_array *stream_rules = nullptr;
    string_array *time_rules = nullptr;
    std::string *sub_info = nullptr;
    bool authorized = false;
    string_icase_map *request_header = nullptr;
    qjs::Runtime *js_runtime = nullptr;
    qjs::Context *js_context = nullptr;
};

int addNodes(std::string link, std::vector<Proxy> &allNodes, int groupID, parse_settings &parse_set);
void filterNodes(std::vector<Proxy> &nodes, string_array &exclude_remarks, string_array &include_remarks, int groupID);
bool applyMatcher(const std::string &rule, std::string &real_rule, const Proxy &node);

#endif // NODEMANIP_H_INCLUDED
