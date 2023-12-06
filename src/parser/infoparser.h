#ifndef INFOPARSER_H_INCLUDED
#define INFOPARSER_H_INCLUDED

#include <string>

#include "utils/string.h"
#include "config/proxy.h"
#include "config/regmatch.h"

bool getSubInfoFromHeader(const std::string &header, std::string &result);
bool getSubInfoFromNodes(const std::vector<Proxy> &nodes, const RegexMatchConfigs &stream_rules, const RegexMatchConfigs &time_rules, std::string &result);
bool getSubInfoFromSSD(const std::string &sub, std::string &result);
unsigned long long streamToInt(const std::string &stream);


#endif // INFOPARSER_H_INCLUDED
