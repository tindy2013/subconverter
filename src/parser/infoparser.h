#ifndef INFOPARSER_H_INCLUDED
#define INFOPARSER_H_INCLUDED

#include <string>

#include "../utils/string.h"
#include "config/proxy.h"

bool getSubInfoFromHeader(const std::string &header, std::string &result);
bool getSubInfoFromNodes(const std::vector<Proxy> &nodes, const string_array &stream_rules, const string_array &time_rules, std::string &result);
bool getSubInfoFromSSD(const std::string &sub, std::string &result);
unsigned long long streamToInt(const std::string &stream);


#endif // INFOPARSER_H_INCLUDED
