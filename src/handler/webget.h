#ifndef WEBGET_H_INCLUDED
#define WEBGET_H_INCLUDED

#include <string>
#include <map>

#include "utils/map_extra.h"
#include "utils/string.h"

enum http_method
{
    HTTP_GET,
    HTTP_HEAD,
    HTTP_POST,
    HTTP_PATCH
};

struct FetchArgument
{
    const http_method method;
    const std::string url;
    const std::string proxy;
    const std::string *post_data = nullptr;
    const string_icase_map *request_headers = nullptr;
    std::string *cookies = nullptr;
    const unsigned int cache_ttl = 0;
    const bool keep_resp_on_fail = false;
};

struct FetchResult
{
    int *status_code;
    std::string *content = nullptr;
    std::string *response_headers = nullptr;
    std::string *cookies = nullptr;
};

int webGet(const FetchArgument& argument, FetchResult &result);
std::string webGet(const std::string &url, const std::string &proxy = "", unsigned int cache_ttl = 0, std::string *response_headers = nullptr, string_icase_map *request_headers = nullptr);
void flushCache();
int webPost(const std::string &url, const std::string &data, const std::string &proxy, const string_icase_map &request_headers, std::string *retData);
int webPatch(const std::string &url, const std::string &data, const std::string &proxy, const string_icase_map &request_headers, std::string *retData);
std::string buildSocks5ProxyString(const std::string &addr, int port, const std::string &username, const std::string &password);

// Unimplemented: (CURLOPT_HTTPHEADER: Host:)
std::string httpGet(const std::string &host, const std::string &addr, const std::string &uri);
std::string httpsGet(const std::string &host, const std::string &addr, const std::string &uri);

#endif // WEBGET_H_INCLUDED
