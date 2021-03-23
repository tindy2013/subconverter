#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <string>

#include "string.h"

std::string getFormData(const std::string &raw_data);
std::string getUrlArg(const std::string &url, const std::string &request);
bool isIPv4(const std::string &address);
bool isIPv6(const std::string &address);
void urlParse(std::string &url, std::string &host, std::string &path, int &port, bool &isTLS);
std::string hostnameToIPAddr(const std::string &host);

inline bool isLink(const std::string &url)
{
    return startsWith(url, "https://") || startsWith(url, "http://") || startsWith(url, "data:");
}

#endif // NETWORK_H_INCLUDED
