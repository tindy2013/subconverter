#include <iostream>
#include <unistd.h>

#include <curl/curl.h>

#include "webget.h"

extern bool print_debug_info;

//std::string user_agent_str = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36";
static std::string user_agent_str = "subconverter/latest cURL/7.xx-DEV";

static int writer(char *data, size_t size, size_t nmemb, std::string *writerData)
{
    if(writerData == NULL)
        return 0;

    writerData->append(data, size*nmemb);

    return size * nmemb;
}

std::string curlGet(std::string url, std::string proxy, std::string &response_headers)
{
    CURL *curl_handle;
    std::string data;

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.data());
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, print_debug_info ? 1L : 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent_str.data());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &response_headers);
    if(proxy != "")
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.data());

    curl_easy_perform(curl_handle);
    curl_easy_cleanup(curl_handle);

    return data;
}

std::string buildSocks5ProxyString(std::string addr, int port, std::string username, std::string password)
{
    std::string authstr = username != "" && password != "" ? username + ":" + password + "@" : "";
    std::string proxystr = "socks5://" + authstr + addr + ":" + std::to_string(port);
    return proxystr;
}

std::string webGet(std::string url, std::string proxy, std::string &response_headers)
{
    return curlGet(url, proxy, response_headers);
}

std::string webGet(std::string url, std::string proxy)
{
    std::string dummy;
    return curlGet(url, proxy, dummy);
}

int curlPost(std::string url, std::string data, std::string proxy, std::string auth_token, std::string *retData)
{
    CURL *curl_handle;
    struct curl_slist *list = NULL;
    int retVal = 0;

    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();
    list = curl_slist_append(list, "Content-Type: application/json;charset='utf-8'");
    if(auth_token.size())
        list = curl_slist_append(list, std::string("Authorization: token " + auth_token).data());

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.data());
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, print_debug_info ? 1L : 0L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, retData);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent_str.data());
    if(proxy != "")
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.data());

    res = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);

    if(res == CURLE_OK)
    {
        res = curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &retVal);
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    return retVal;
}

int curlPatch(std::string url, std::string data, std::string proxy, std::string auth_token, std::string *retData)
{
    CURL *curl_handle;
    int retVal = 0;
    struct curl_slist *list = NULL;

    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();

    list = curl_slist_append(list, "Content-Type: application/json;charset='utf-8'");
    if(auth_token.size())
        list = curl_slist_append(list, std::string("Authorization: token " + auth_token).data());

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.data());
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, print_debug_info ? 1L : 0L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, retData);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent_str.data());
    if(proxy != "")
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.data());

    res = curl_easy_perform(curl_handle);
    curl_slist_free_all(list);
    if(res == CURLE_OK)
    {
        res = curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &retVal);
    }

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    return retVal;
}
