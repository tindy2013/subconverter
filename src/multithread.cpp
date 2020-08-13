#include <future>
#include <thread>
#include "webget.h"
#include "multithread.h"

//safety lock for multi-thread
std::mutex on_emoji, on_rename, on_stream, on_time;

extern string_array gEmojis, gRenames;
extern string_array gStreamNodeRules, gTimeNodeRules;

string_array safe_get_emojis()
{
    guarded_mutex guard(on_emoji);
    return gEmojis;
}

string_array safe_get_renames()
{
    guarded_mutex guard(on_rename);
    return gRenames;
}

string_array safe_get_streams()
{
    guarded_mutex guard(on_stream);
    return gStreamNodeRules;
}

string_array safe_get_times()
{
    guarded_mutex guard(on_time);
    return gTimeNodeRules;
}

void safe_set_emojis(string_array &data)
{
    guarded_mutex guard(on_emoji);
    gEmojis.swap(data);
}

void safe_set_renames(string_array &data)
{
    guarded_mutex guard(on_rename);
    gRenames.swap(data);
}

void safe_set_streams(string_array &data)
{
    guarded_mutex guard(on_stream);
    gStreamNodeRules.swap(data);
}

void safe_set_times(string_array &data)
{
    guarded_mutex guard(on_time);
    gTimeNodeRules.swap(data);
}

std::shared_future<std::string> fetchFileAsync(const std::string &path, const std::string &proxy, int cache_ttl, bool async)
{
    std::shared_future<std::string> retVal;
    if(fileExist(path, true))
        retVal = std::async(std::launch::async, [path](){return fileGet(path, true);});
    else if(isLink(path))
        retVal = std::async(std::launch::async, [path, proxy, cache_ttl](){return webGet(path, proxy, cache_ttl);});
    else
        return std::async(std::launch::async, [](){return std::string();});
    if(!async)
        retVal.wait();
    return retVal;
}

std::string fetchFile(const std::string &path, const std::string &proxy, int cache_ttl)
{
    return fetchFileAsync(path, proxy, cache_ttl, false).get();
}
