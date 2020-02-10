#ifndef INTERFACES_H_INCLUDED
#define INTERFACES_H_INCLUDED

#include <string>
#include <map>

#include "subexport.h"
#include "webserver.h"

void refreshRulesets(string_array &ruleset_list, std::vector<ruleset_content> &rca);
void readConf();
void generateBase();
std::string subconverter(RESPONSE_CALLBACK_ARGS);
std::string simpleToClashR(RESPONSE_CALLBACK_ARGS);
std::string surgeConfToClash(RESPONSE_CALLBACK_ARGS);

#endif // INTERFACES_H_INCLUDED
