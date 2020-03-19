#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <mutex>

#include <curl/curl.h>

#include "webget.h"
#include "version.h"
#include "misc.h"
#include "logger.h"

#ifdef _WIN32
#ifndef _stat
#define _stat stat
#endif // _stat
#endif // _WIN32

extern bool print_debug_info;
extern int global_log_level;

typedef std::lock_guard<std::mutex> guarded_mutex;
std::mutex cache_rw_lock;

//std::string user_agent_str = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36";
std::string user_agent_str = "subconverter/" + std::string(VERSION) + " cURL/" + std::string(LIBCURL_VERSION);

static inline void curl_init()
{
    static bool init = false;
    if(!init)
    {
        curl_global_init(CURL_GLOBAL_ALL);
        init = true;
    }
}

static int writer(char *data, size_t size, size_t nmemb, std::string *writerData)
{
    if(writerData == NULL)
        return 0;

    writerData->append(data, size*nmemb);

    return size * nmemb;
}

static inline void curl_set_common_options(CURL *curl_handle, const char *url)
{
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, global_log_level == LOG_LEVEL_VERBOSE ? 1L : 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent_str.data());
}

static std::string curlGet(std::string url, std::string proxy, std::string &response_headers, CURLcode &return_code)
{
    CURL *curl_handle;
    std::string data;
    long retVal = 0;

    curl_init();

    curl_handle = curl_easy_init();
    curl_set_common_options(curl_handle, url.data());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, writer);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &response_headers);
    if(proxy != "")
        curl_easy_setopt(curl_handle, CURLOPT_PROXY, proxy.data());

    return_code = curl_easy_perform(curl_handle);
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &retVal);
    curl_easy_cleanup(curl_handle);

    if(return_code != CURLE_OK || retVal != 200)
        data.clear();
    data.shrink_to_fit();

    return data;
}

// data:[<mediatype>][;base64],<data>
static std::string dataGet(std::string url)
{
    if (!startsWith(url, "data:"))
        return "";
    std::string::size_type comma = url.find(',');
    if (comma == std::string::npos)
        return "";

    std::string data = UrlDecode(url.substr(comma));
    if (endsWith(url.substr(0, comma), ";base64")) {
        return urlsafe_base64_decode(data);
    } else {
        return data;
    }
}

std::string buildSocks5ProxyString(std::string addr, int port, std::string username, std::string password)
{
    std::string authstr = username != "" && password != "" ? username + ":" + password + "@" : "";
    std::string proxystr = "socks5://" + authstr + addr + ":" + std::to_string(port);
    return proxystr;
}

std::string webGet(std::string url, std::string proxy, std::string &response_headers, unsigned int cache_ttl)
{
    std::string content;
    CURLcode return_code;
    if (startsWith(url, "data:"))
        return dataGet(url);
    // cache system
    if(cache_ttl > 0)
    {
        md("cache");
        const std::string url_md5 = getMD5(url);
        const std::string path = "cache/" + url_md5, path_header = path + "_header";
        struct stat result;
        if(stat(path.data(), &result) == 0) // cache exist
        {
            time_t mtime = result.st_mtime, now = time(NULL); // get cache modified time and current time
            if(difftime(now, mtime) <= cache_ttl) // within TTL
            {
                writeLog(0, "CACHE HIT: '" + url + "', using local cache.");
                guarded_mutex guard(cache_rw_lock);
                response_headers = fileGet(path_header, true);
                return fileGet(path, true);
            }
            writeLog(0, "CACHE MISS: '" + url + "', TTL timeout, creating new cache."); // out of TTL
        }
        else
            writeLog(0, "CACHE NOT EXIST: '" + url + "', creating new cache.");
        content = curlGet(url, proxy, response_headers, return_code); // try to fetch data
        if(return_code == CURLE_OK) // success, save new cache
        {
            guarded_mutex guard(cache_rw_lock);
            fileWrite(path, content, true);
            fileWrite(path_header, response_headers, true);
        }
        else
        {
            if(fileExist(path)) // failed, check if cache exist
            {
                writeLog(0, "Fetch failed. Serving cached content."); // cache exist, serving cache
                guarded_mutex guard(cache_rw_lock);
                content = fileGet(path, true);
                response_headers = fileGet(path_header, true);
            }
            else
                writeLog(0, "Fetch failed. No local cache available."); // cache not exist, serving nothing
        }
        return content;
    }
    return curlGet(url, proxy, response_headers, return_code);
}

std::string webGet(std::string url, std::string proxy)
{
    std::string dummy;
    return webGet(url, proxy, dummy);
}

std::string webGet(std::string url, std::string proxy, unsigned int cache_ttl)
{
    std::string dummy;
    return webGet(url, proxy, dummy, cache_ttl);
}

int curlPost(std::string url, std::string data, std::string proxy, std::string auth_token, std::string *retData)
{
    CURL *curl_handle;
    CURLcode res;
    struct curl_slist *list = NULL;
    long retVal = 0;

    curl_init();
    curl_handle = curl_easy_init();
    list = curl_slist_append(list, "Content-Type: application/json;charset='utf-8'");
    if(auth_token.size())
        list = curl_slist_append(list, std::string("Authorization: token " + auth_token).data());

    curl_set_common_options(curl_handle, url.data());
    curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data.size());
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

    return retVal;
}

int webPost(std::string url, std::string data, std::string proxy, std::string auth_token, std::string *retData)
{
    return curlPost(url, data, proxy, auth_token, retData);
}

int curlPatch(std::string url, std::string data, std::string proxy, std::string auth_token, std::string *retData)
{
    CURL *curl_handle;
    CURLcode res;
    long retVal = 0;
    struct curl_slist *list = NULL;

    curl_init();

    curl_handle = curl_easy_init();

    list = curl_slist_append(list, "Content-Type: application/json;charset='utf-8'");
    if(auth_token.size())
        list = curl_slist_append(list, std::string("Authorization: token " + auth_token).data());

    curl_set_common_options(curl_handle, url.data());
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, data.size());
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

    return retVal;
}

int webPatch(std::string url, std::string data, std::string proxy, std::string auth_token, std::string *retData)
{
    return curlPatch(url, data, proxy, auth_token, retData);
}
