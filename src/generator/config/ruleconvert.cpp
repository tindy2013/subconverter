#include <string>

#include "../../handler/settings.h"
#include "../../utils/logger.h"
#include "../../utils/network.h"
#include "../../utils/regexp.h"
#include "../../utils/string.h"
#include "subexport.h"

/// rule type lists
#define basic_types "DOMAIN", "DOMAIN-SUFFIX", "DOMAIN-KEYWORD", "IP-CIDR", "SRC-IP-CIDR", "GEOIP", "MATCH", "FINAL"
string_array ClashRuleTypes = {basic_types, "IP-CIDR6", "SRC-PORT", "DST-PORT", "PROCESS-NAME"};
string_array Surge2RuleTypes = {basic_types, "IP-CIDR6", "USER-AGENT", "URL-REGEX", "PROCESS-NAME", "IN-PORT", "DEST-PORT", "SRC-IP"};
string_array SurgeRuleTypes = {basic_types, "IP-CIDR6", "USER-AGENT", "URL-REGEX", "AND", "OR", "NOT", "PROCESS-NAME", "IN-PORT", "DEST-PORT", "SRC-IP"};
string_array QuanXRuleTypes = {basic_types, "USER-AGENT", "HOST", "HOST-SUFFIX", "HOST-KEYWORD"};
string_array SurfRuleTypes = {basic_types, "IP-CIDR6", "PROCESS-NAME", "IN-PORT", "DEST-PORT", "SRC-IP"};

std::string convertRuleset(const std::string &content, int type)
{
    /// Target: Surge type,pattern[,flag]
    /// Source: QuanX type,pattern[,group]
    ///         Clash payload:\n  - 'ipcidr/domain/classic(Surge-like)'

    std::string output, strLine;

    if(type == RULESET_SURGE)
        return content;

    if(regFind(content, "^payload:\\r?\\n")) /// Clash
    {
        output = regReplace(regReplace(content, "payload:\\r?\\n", "", true), "\\s?^\\s*-\\s+('|\"?)(.*)\\1$", "\n$2", true);
        if(type == RULESET_CLASH_CLASSICAL) /// classical type
            return output;
        std::stringstream ss;
        ss << output;
        char delimiter = getLineBreak(output);
        output.clear();
        string_size pos, lineSize;
        while(getline(ss, strLine, delimiter))
        {
            strLine = trim(strLine);
            lineSize = strLine.size();
            if(lineSize && strLine[lineSize - 1] == '\r') //remove line break
                strLine.erase(--lineSize);

            if(strFind(strLine, "//"))
            {
                strLine.erase(strLine.find("//"));
                strLine = trimWhitespace(strLine);
            }

            if(!strLine.empty() && (strLine[0] != ';' && strLine[0] != '#' && !(lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')))
            {
                pos = strLine.find("/");
                if(pos != strLine.npos) /// ipcidr
                {
                    if(isIPv4(strLine.substr(0, pos)))
                        output += "IP-CIDR,";
                    else
                        output += "IP-CIDR6,";
                }
                else
                {
                    if(strLine[0] == '.' || (lineSize >= 2 && strLine[0] == '+' && strLine[1] == '.')) /// suffix
                    {
                        bool keyword_flag = false;
                        while(endsWith(strLine, ".*"))
                        {
                            keyword_flag = true;
                            strLine.erase(strLine.size() - 2);
                        }
                        output += "DOMAIN-";
                        if(keyword_flag)
                            output += "KEYWORD,";
                        else
                            output += "SUFFIX,";
                        strLine.erase(0, 2 - (strLine[0] == '.'));
                    }
                    else
                        output += "DOMAIN,";
                }
            }
            output += strLine;
            output += '\n';
        }
        return output;
    }
    else /// QuanX
    {
        output = regReplace(regReplace(content, "^(?i:host)", "DOMAIN", true), "^(?i:ip6-cidr)", "IP-CIDR6", true); //translate type
        output = regReplace(output, "^((?i:DOMAIN(?:-(?:SUFFIX|KEYWORD))?|IP-CIDR6?|USER-AGENT),)\\s*?(\\S*?)(?:,(?!no-resolve).*?)(,no-resolve)?$", "\\U$1\\E$2${3:-}", true); //remove group
        return output;
    }
}


void rulesetToClash(YAML::Node &base_rule, std::vector<RulesetContent> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name)
{
    string_array allRules;
    std::string rule_group, retrieved_rules, strLine;
    std::stringstream strStrm;
    const std::string field_name = new_field_name ? "rules" : "Rule";
    YAML::Node rules;
    size_t total_rules = 0;

    if(!overwrite_original_rules && base_rule[field_name].IsDefined())
        rules = base_rule[field_name];

    for(RulesetContent &x : ruleset_content_array)
    {
        if(global.maxAllowedRules && total_rules > global.maxAllowedRules)
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
            if(global.maxAllowedRules && total_rules > global.maxAllowedRules)
                break;
            strLine = trimWhitespace(strLine, true, true); //remove whitespaces
            lineSize = strLine.size();
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            if(std::none_of(ClashRuleTypes.begin(), ClashRuleTypes.end(), [strLine](std::string type){return startsWith(strLine, type);}))
                continue;
            if(strFind(strLine, "//"))
            {
                strLine.erase(strLine.find("//"));
                strLine = trimWhitespace(strLine);
            }
            strLine += "," + rule_group;
            if(count_least(strLine, ',', 3))
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            allRules.emplace_back(std::move(strLine));
            //rules.push_back(strLine);
        }
    }

    for(std::string &x : allRules)
    {
        rules.push_back(x);
    }

    base_rule[field_name] = rules;
}

std::string rulesetToClashStr(YAML::Node &base_rule, std::vector<RulesetContent> &ruleset_content_array, bool overwrite_original_rules, bool new_field_name)
{
    std::string rule_group, retrieved_rules, strLine;
    std::stringstream strStrm;
    const std::string field_name = new_field_name ? "rules" : "Rule";
    std::string output_content = "\n" + field_name + ":\n";
    size_t total_rules = 0;

    if(!overwrite_original_rules && base_rule[field_name].IsDefined())
    {
        for(size_t i = 0; i < base_rule[field_name].size(); i++)
            output_content += "  - " + safe_as<std::string>(base_rule[field_name][i]) + "\n";
    }
    base_rule.remove(field_name);

    for(RulesetContent &x : ruleset_content_array)
    {
        if(global.maxAllowedRules && total_rules > global.maxAllowedRules)
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
            output_content += "  - " + strLine + "\n";
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
            if(global.maxAllowedRules && total_rules > global.maxAllowedRules)
                break;
            strLine = trimWhitespace(strLine, true, true); //remove whitespaces
            lineSize = strLine.size();
            if(!lineSize || strLine[0] == ';' || strLine[0] == '#' || (lineSize >= 2 && strLine[0] == '/' && strLine[1] == '/')) //empty lines and comments are ignored
                continue;
            if(std::none_of(ClashRuleTypes.begin(), ClashRuleTypes.end(), [strLine](std::string type){ return startsWith(strLine, type); }))
                continue;
            if(strFind(strLine, "//"))
            {
                strLine.erase(strLine.find("//"));
                strLine = trimWhitespace(strLine);
            }
            strLine += "," + rule_group;
            if(count_least(strLine, ',', 3))
                strLine = regReplace(strLine, "^(.*?,.*?)(,.*)(,.*)$", "$1$3$2");
            output_content += "  - " + strLine + "\n";
            total_rules++;
        }
    }
    return output_content;
}

void rulesetToSurge(INIReader &base_rule, std::vector<RulesetContent> &ruleset_content_array, int surge_ver, bool overwrite_original_rules, std::string remote_path_prefix)
{
    string_array allRules;
    std::string rule_group, rule_path, rule_path_typed, retrieved_rules, strLine;
    std::stringstream strStrm;
    size_t total_rules = 0;

    switch(surge_ver) //other version: -3 for Surfboard, -4 for Loon
    {
    case 0:
        base_rule.set_current_section("RoutingRule"); //Mellow
        break;
    case -1:
        base_rule.set_current_section("filter_local"); //Quantumult X
        break;
    case -2:
        base_rule.set_current_section("TCP"); //Quantumult
        break;
    default:
        base_rule.set_current_section("Rule");
    }

    if(overwrite_original_rules)
    {
        base_rule.erase_section();
        switch(surge_ver)
        {
        case -1:
            base_rule.erase_section("filter_remote");
            break;
        case -4:
            base_rule.erase_section("Remote Rule");
            break;
        }
    }

    const std::string rule_match_regex = "^(.*?,.*?)(,.*)(,.*)$";

    for(RulesetContent &x : ruleset_content_array)
    {
        if(global.maxAllowedRules && total_rules > global.maxAllowedRules)
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
            strLine = replaceAllDistinct(strLine, ",,", ",");
            allRules.emplace_back(std::move(strLine));
            total_rules++;
            continue;
        }
        else
        {
            if(surge_ver == -1 && x.rule_type == RULESET_QUANX && isLink(rule_path))
            {
                strLine = rule_path + ", tag=" + rule_group + ", force-policy=" + rule_group + ", enabled=true";
                base_rule.set("filter_remote", "{NONAME}", strLine);
                continue;
            }
            if(fileExist(rule_path))
            {
                if(surge_ver > 2 && remote_path_prefix.size())
                {
                    strLine = "RULE-SET," + remote_path_prefix + "/getruleset?type=1&url=" + urlSafeBase64Encode(rule_path_typed) + "," + rule_group;
                    if(x.update_interval)
                        strLine += ",update-interval=" + std::to_string(x.update_interval);
                    allRules.emplace_back(std::move(strLine));
                    continue;
                }
                else if(surge_ver == -1 && remote_path_prefix.size())
                {
                    strLine = remote_path_prefix + "/getruleset?type=2&url=" + urlSafeBase64Encode(rule_path_typed) + "&group=" + urlSafeBase64Encode(rule_group);
                    strLine += ", tag=" + rule_group + ", enabled=true";
                    base_rule.set("filter_remote", "{NONAME}", strLine);
                    continue;
                }
                else if(surge_ver == -4 && remote_path_prefix.size())
                {
                    strLine = remote_path_prefix + "/getruleset?type=1&url=" + urlSafeBase64Encode(rule_path_typed) + "," + rule_group;
                    base_rule.set("Remote Rule", "{NONAME}", strLine);
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
                            strLine = "RULE-SET," + remote_path_prefix + "/getruleset?type=1&url=" + urlSafeBase64Encode(rule_path_typed) + "," + rule_group;
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
                    strLine = remote_path_prefix + "/getruleset?type=2&url=" + urlSafeBase64Encode(rule_path_typed) + "&group=" + urlSafeBase64Encode(rule_group);
                    strLine += ", tag=" + rule_group + ", enabled=true";
                    base_rule.set("filter_remote", "{NONAME}", strLine);
                    continue;
                }
                else if(surge_ver == -4)
                {
                    strLine = rule_path + "," + rule_group;
                    base_rule.set("Remote Rule", "{NONAME}", strLine);
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
                if(global.maxAllowedRules && total_rules > global.maxAllowedRules)
                    break;
                strLine = trimWhitespace(strLine, true, true);
                lineSize = strLine.size();
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

                if(strFind(strLine, "//"))
                {
                    strLine.erase(strLine.find("//"));
                    strLine = trimWhitespace(strLine);
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
        base_rule.set("{NONAME}", x);
    }
}
