#ifndef WEBGET_H_INCLUDED
#define WEBGET_H_INCLUDED

#include <string>
#include <map>

#include "misc.h"

struct FetchArgument
{
    const std::string url;
    const std::string proxy;
    string_map *request_headers = NULL;
    const unsigned int cache_ttl = 0;
};

struct FetchResult
{
    int *status_code;
    std::string *content = NULL;
    std::string *response_headers = NULL;
};

std::string webGet(const std::string &url, const std::string &proxy = "", unsigned int cache_ttl = 0, std::string *response_headers = NULL, string_map *request_headers = NULL);
int webPost(const std::string &url, const std::string &data, const std::string &proxy, const string_array &request_headers, std::string *retData);
int webPatch(const std::string &url, const std::string &data, const std::string &proxy, const string_array &request_headers, std::string *retData);
std::string buildSocks5ProxyString(const std::string &addr, int port, const std::string &username, const std::string &password);

// Unimplemented: (CURLOPT_HTTPHEADER: Host:)
std::string httpGet(const std::string &host, const std::string &addr, const std::string &uri);
std::string httpsGet(const std::string &host, const std::string &addr, const std::string &uri);

static inline bool isLink(const std::string &url)
{
    return startsWith(url, "https://") || startsWith(url, "http://") || startsWith(url, "data:");
}

#endif // WEBGET_H_INCLUDED
