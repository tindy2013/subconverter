#ifndef INTERFACES_H_INCLUDED
#define INTERFACES_H_INCLUDED

#include <string>
#include <map>
#include <inja.hpp>

#include "../config/ruleset.h"
#include "../generator/config/subexport.h"
#include "../server/webserver.h"

std::string parseProxy(const std::string &source);

void refreshRulesets(RulesetConfigs &ruleset_list, std::vector<RulesetContent> &rca);
void readConf();
int simpleGenerator();
std::string convertRuleset(const std::string &content, int type);

std::string getConvertedRuleset(RESPONSE_CALLBACK_ARGS);
std::string getProfile(RESPONSE_CALLBACK_ARGS);
std::string getRuleset(RESPONSE_CALLBACK_ARGS);

std::string subconverter(RESPONSE_CALLBACK_ARGS);
std::string simpleToClashR(RESPONSE_CALLBACK_ARGS);
std::string surgeConfToClash(RESPONSE_CALLBACK_ARGS);

std::string renderTemplate(RESPONSE_CALLBACK_ARGS);

std::string template_webGet(inja::Arguments &args);
std::string jinja2_webGet(const std::string &url);
std::string parseHostname(inja::Arguments &args);

#endif // INTERFACES_H_INCLUDED
