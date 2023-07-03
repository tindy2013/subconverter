#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
//#include <mutex>
#include <thread>
#include <atomic>

#include <curl/curl.h>

#include "../handler/settings.h"
#include "../utils/base64/base64.h"
#include "../utils/defer.h"
#include "../utils/file_extra.h"
#include "../utils/logger.h"
#include "../utils/urlencode.h"
#include "../version.h"
#include "webget.h"

#ifdef _WIN32
#ifndef _stat
#define _stat stat
#endif // _stat
#endif // _WIN32

/*
using guarded_mutex = std::lock_guard<std::mutex>;
std::mutex cache_rw_lock;
*/

class RWLock
{
#define WRITE_LOCK_STATUS -1
#define FREE_STATUS 0
private:
    const std::thread::id NULL_THREAD;
    const bool WRITE_FIRST;
    std::thread::id m_write_thread_id;
    std::atomic_int m_lockCount;
    std::atomic_uint m_writeWaitCount;
public:
    RWLock(const RWLock&) = delete;
    RWLock& operator=(const RWLock&) = delete;
    RWLock(bool writeFirst = true): WRITE_FIRST(writeFirst), m_write_thread_id(), m_lockCount(0), m_writeWaitCount(0) {}
    virtual ~RWLock() = default;
    int readLock()
    {
        if (std::this_thread::get_id() != m_write_thread_id)
        {
            int count;
            if (WRITE_FIRST)
                do {
                    while ((count = m_lockCount) == WRITE_LOCK_STATUS || m_writeWaitCount > 0);
                } while (!m_lockCount.compare_exchange_weak(count, count + 1));
            else
                do {
                    while ((count = m_lockCount) == WRITE_LOCK_STATUS);
                } while (!m_lockCount.compare_exchange_weak(count, count + 1));
        }
        return m_lockCount;
    }
    int readUnlock()
    {
        if (std::this_thread::get_id() != m_write_thread_id)
            --m_lockCount;
        return m_lockCount;
    }
    int writeLock()
    {
        if (std::this_thread::get_id() != m_write_thread_id)
        {
            ++m_writeWaitCount;
            for (int zero = FREE_STATUS; !m_lockCount.compare_exchange_weak(zero, WRITE_LOCK_STATUS); zero = FREE_STATUS);
            --m_writeWaitCount;
            m_write_thread_id = std::this_thread::get_id();
        }
        return m_lockCount;
    }
    int writeUnlock()
    {
        if (std::this_thread::get_id() != m_write_thread_id)
        {
            throw std::runtime_error("writeLock/Unlock mismatch");
        }
        if (WRITE_LOCK_STATUS != m_lockCount)
        {
            throw std::runtime_error("RWLock internal error");
        }
        m_write_thread_id = NULL_THREAD;
        m_lockCount.store(FREE_STATUS);
        return m_lockCount;
    }
};

RWLock cache_rw_lock;

//std::string user_agent_str = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36";
static std::string user_agent_str = "subconverter/" VERSION " cURL/" LIBCURL_VERSION;

struct curl_progress_data
{
    long size_limit = 0L;
};

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

static int dummy_writer(char *data, size_t size, size_t nmemb, void *writerData)
{
    /// dummy writer, do not save anything
    (void)data;
    (void)writerData;
    return size * nmemb;
}

static int size_checker(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    if(clientp)
    {
        curl_progress_data *data = reinterpret_cast<curl_progress_data*>(clientp);
        if(data->size_limit)
        {
            if(dlnow > data->size_limit)
                return 1;
        }
    }
    return 0;
}

static inline void curl_set_common_options(CURL *curl_handle, const char *url, curl_progress_data *data)
{
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, global.logLevel == LOG_LEVEL_VERBOSE ? 1L : 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 20L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, user_agent_str.data());
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
    if(data)
    {
        if(data->size_limit)
            curl_easy_setopt(curl_handle, CURLOPT_MAXFILESIZE, data->size_limit);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, size_checker);
        curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, data);
    }
}

//static std::string curlGet(const std::string &url, const std::string &proxy, std::string &response_headers, CURLcode &return_code, const string_map &request_headers)
static int curlGet(const FetchArgument &argument, FetchResult &result)
{
    CURL *curl_handle;
    std::string *data = result.content, new_url = argument.url;
    struct curl_slist *list = NULL;
    defer(curl_slist_free_all(list);)
    long retVal = 0;

    curl_init();

    curl_handle = curl_easy_init();
    if(argument.proxy.size())
    {
        if(startsWith(argument.proxy, "cors:"))
        {
            list = curl_slist_append(list, "X-Requested-With: subconverter " VERSION);
            new_url = argument.proxy.substr(5) + argument.url;
        }
        else
            curl_easy_setopt(curl_handle, CURLOPT_PROXY, argument.proxy.data());
    }
    curl_progress_data limit;
    limit.size_limit = global.maxAllowedDownloadSize;
    curl_set_common_options(curl_handle, new_url.data(), &limit);
    list = curl_slist_append(list, "Content-Type: application/json;charset=utf-8");
    if(argument.request_headers)
    {
        for(auto &x : *argument.request_headers)
            list = curl_slist_append(list, (x.first + ": " + x.second).data());
    }
    list = curl_slist_append(list, "SubConverter-Request: 1");
    list = curl_slist_append(list, "SubConverter-Version: " VERSION);
    if(list)
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, list);

    if(result.content)
    {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, writer);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, result.content);
    }
    else
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, dummy_writer);
    if(result.response_headers)
    {
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, writer);
        curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, result.response_headers);
    }
    else
        curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, dummy_writer);

    if(argument.cookies)
    {
        string_array cookies = split(*argument.cookies, "\r\n");
        for(auto &x : cookies)
            curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, x.c_str());
    }

    switch(argument.method)
    {
    case HTTP_POST:
        curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
        if(argument.post_data)
        {
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, argument.post_data->data());
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, argument.post_data->size());
        }
        break;
    case HTTP_PATCH:
        curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, "PATCH");
        if(argument.post_data)
        {
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, argument.post_data->data());
            curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, argument.post_data->size());
        }
        break;
    case HTTP_HEAD:
        curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 1L);
        break;
    case HTTP_GET:
        break;
    }

    unsigned int fail_count = 0, max_fails = 1;
    while(true)
    {
        retVal = curl_easy_perform(curl_handle);
        if(retVal == CURLE_OK || max_fails >= fail_count)
            break;
        else
            fail_count++;
    }

    long code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_HTTP_CODE, &code);
    *result.status_code = code;

    if(result.cookies)
    {
        curl_slist *cookies = nullptr;
        curl_easy_getinfo(curl_handle, CURLINFO_COOKIELIST, &cookies);
        if(cookies)
        {
            auto each = cookies;
            while(each)
            {
                result.cookies->append(each->data);
                *result.cookies += "\r\n";
                each = each->next;
            }
        }
        curl_slist_free_all(cookies);
    }

    curl_easy_cleanup(curl_handle);

    if(data && !argument.keep_resp_on_fail)
    {
        if(retVal != CURLE_OK || *result.status_code != 200)
            data->clear();
        data->shrink_to_fit();
    }

    return *result.status_code;
}

// data:[<mediatype>][;base64],<data>
static std::string dataGet(const std::string &url)
{
    if (!startsWith(url, "data:"))
        return "";
    std::string::size_type comma = url.find(',');
    if (comma == std::string::npos || comma == url.size() - 1)
        return "";

    std::string data = urlDecode(url.substr(comma + 1));
    if (endsWith(url.substr(0, comma), ";base64")) {
        return urlSafeBase64Decode(data);
    } else {
        return data;
    }
}

std::string buildSocks5ProxyString(const std::string &addr, int port, const std::string &username, const std::string &password)
{
    std::string authstr = username.size() && password.size() ? username + ":" + password + "@" : "";
    std::string proxystr = "socks5://" + authstr + addr + ":" + std::to_string(port);
    return proxystr;
}

std::string webGet(const std::string &url, const std::string &proxy, unsigned int cache_ttl, std::string *response_headers, string_icase_map *request_headers)
{
    int return_code = 0;
    std::string content;

    FetchArgument argument {HTTP_GET, url, proxy, nullptr, request_headers, nullptr, cache_ttl};
    FetchResult fetch_res {&return_code, &content, response_headers, nullptr};

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
                //guarded_mutex guard(cache_rw_lock);
                cache_rw_lock.readLock();
                defer(cache_rw_lock.readUnlock();)
                if(response_headers)
                    *response_headers = fileGet(path_header, true);
                return fileGet(path, true);
            }
            writeLog(0, "CACHE MISS: '" + url + "', TTL timeout, creating new cache."); // out of TTL
        }
        else
            writeLog(0, "CACHE NOT EXIST: '" + url + "', creating new cache.");
        //content = curlGet(url, proxy, response_headers, return_code); // try to fetch data
        curlGet(argument, fetch_res);
        if(return_code == 200) // success, save new cache
        {
            //guarded_mutex guard(cache_rw_lock);
            cache_rw_lock.writeLock();
            defer(cache_rw_lock.writeUnlock();)
            fileWrite(path, content, true);
            if(response_headers)
                fileWrite(path_header, *response_headers, true);
        }
        else
        {
            if(fileExist(path) && global.serveCacheOnFetchFail) // failed, check if cache exist
            {
                writeLog(0, "Fetch failed. Serving cached content."); // cache exist, serving cache
                //guarded_mutex guard(cache_rw_lock);
                cache_rw_lock.readLock();
                defer(cache_rw_lock.readUnlock();)
                content = fileGet(path, true);
                if(response_headers)
                    *response_headers = fileGet(path_header, true);
            }
            else
                writeLog(0, "Fetch failed. No local cache available."); // cache not exist or not allow to serve cache, serving nothing
        }
        return content;
    }
    //return curlGet(url, proxy, response_headers, return_code);
    curlGet(argument, fetch_res);
    return content;
}

void flushCache()
{
    //guarded_mutex guard(cache_rw_lock);
    cache_rw_lock.writeLock();
    defer(cache_rw_lock.writeUnlock();)
    operateFiles("cache", [](const std::string &file){ remove(("cache/" + file).data()); return 0; });
}

int webPost(const std::string &url, const std::string &data, const std::string &proxy, const string_icase_map &request_headers, std::string *retData)
{
    //return curlPost(url, data, proxy, request_headers, retData);
    int return_code = 0;
    FetchArgument argument {HTTP_POST, url, proxy, &data, &request_headers, nullptr, 0, true};
    FetchResult fetch_res {&return_code, retData, nullptr, nullptr};
    return webGet(argument, fetch_res);
}

int webPatch(const std::string &url, const std::string &data, const std::string &proxy, const string_icase_map &request_headers, std::string *retData)
{
    //return curlPatch(url, data, proxy, request_headers, retData);
    int return_code = 0;
    FetchArgument argument {HTTP_PATCH, url, proxy, &data, &request_headers, nullptr, 0, true};
    FetchResult fetch_res {&return_code, retData, nullptr, nullptr};
    return webGet(argument, fetch_res);
}

int webHead(const std::string &url, const std::string &proxy, const string_icase_map &request_headers, std::string &response_headers)
{
    //return curlHead(url, proxy, request_headers, response_headers);
    int return_code = 0;
    FetchArgument argument {HTTP_HEAD, url, proxy, nullptr, &request_headers, nullptr, 0};
    FetchResult fetch_res {&return_code, nullptr, &response_headers, nullptr};
    return webGet(argument, fetch_res);
}

string_array headers_map_to_array(const string_map &headers)
{
    string_array result;
    for(auto &kv : headers)
        result.push_back(kv.first + ": " + kv.second);
    return result;
}

int webGet(const FetchArgument& argument, FetchResult &result)
{
    return curlGet(argument, result);
}
