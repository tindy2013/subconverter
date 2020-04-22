#ifndef WEBGET_H_INCLUDED
#define WEBGET_H_INCLUDED

#include <string>

#ifdef _WIN32
#include <ws2tcpip.h>
#endif // _WIN32

#include "misc.h"

std::string webGet(const std::string &url, const std::string &proxy);
std::string webGet(const std::string &url, const std::string &proxy, unsigned int cache_ttl);
std::string webGet(const std::string &url, const std::string &proxy, std::string &response_headers, unsigned int cache_ttl = 0);
int webPost(const std::string &url, const std::string &data, const std::string &proxy, const string_array &request_headers, std::string *retData);
int webPatch(const std::string &url, const std::string &data, const std::string &proxy, const string_array &request_headers, std::string *retData);
std::string buildSocks5ProxyString(const std::string &addr, int port, const std::string &username, const std::string &password);

// Unimplemented: (CURLOPT_HTTPHEADER: Host:)
std::string httpGet(const std::string &host, const std::string &addr, const std::string &uri);
std::string httpsGet(const std::string &host, const std::string &addr, const std::string &uri);

#endif // WEBGET_H_INCLUDED
