#ifndef URLENCODE_H_INCLUDED
#define URLENCODE_H_INCLUDED

#include <string>

#include "utils/string.h"

std::string urlEncode(const std::string& str);
std::string urlDecode(const std::string& str);
std::string joinArguments(const string_multimap &args);

#endif // URLENCODE_H_INCLUDED
