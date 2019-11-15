#ifndef WEBGET_H_INCLUDED
#define WEBGET_H_INCLUDED

#include <string>

#ifdef _WIN32
#include <ws2tcpip.h>
#endif // _WIN32

#include "misc.h"

std::string webGet(std::string url, std::string proxy = "");
std::string httpGet(std::string host, std::string addr, std::string uri);
std::string httpsGet(std::string host, std::string addr, std::string uri);
int curlPost(std::string url, std::string data, std::string proxy, std::string auth_token, std::string *retData);
int curlPatch(std::string url, std::string data, std::string proxy, std::string auth_token, std::string *retData);
std::string buildSocks5ProxyString(std::string addr, int port, std::string username, std::string password);

#endif // WEBGET_H_INCLUDED
