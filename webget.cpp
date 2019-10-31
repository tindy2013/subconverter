#include <iostream>
#include <unistd.h>

#include <curl/curl.h>

#include "webget.h"

std::string user_agent_str = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36";

static int writer(char *data, size_t size, size_t nmemb, std::string *writerData)
{
    if(writerData == NULL)
        return 0;

    writerData->append(data, size*nmemb);

    return size * nmemb;
}

std::string curlGet(std::string url, std::string proxy)
{
    CURL *curl_handle;
    std::string data;

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.data());
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent_str.data());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &data);
    if(proxy != "")
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.data());

    curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);

    return data;
}

std::string buildSocks5ProxyString(std::string addr, int port, std::string username, std::string password)
{
    std::string authstr = username != "" && password != "" ? username + ":" + password + "@" : "";
    std::string proxystr = "socks5://" + authstr + addr + ":" + std::__cxx11::to_string(port);
    return proxystr;
}

std::string webGet(std::string url, std::string proxy)
{
    return curlGet(url, proxy);
}
