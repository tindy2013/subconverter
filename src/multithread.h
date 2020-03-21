#ifndef MULTITHREAD_H_INCLUDED
#define MULTITHREAD_H_INCLUDED

#include <mutex>
#include <future>

#include <yaml-cpp/yaml.h>

#include "misc.h"
#include "ini_reader.h"

typedef std::lock_guard<std::mutex> guarded_mutex;

string_array safe_get_emojis();
string_array safe_get_renames();
string_array safe_get_streams();
string_array safe_get_times();
YAML::Node safe_get_clash_base();
INIReader safe_get_mellow_base();
void safe_set_emojis(string_array &data);
void safe_set_renames(string_array &data);
void safe_set_streams(string_array &data);
void safe_set_times(string_array &data);
std::shared_future<std::string> fetchFileAsync(const std::string &path, const std::string &proxy, int cache_ttl, bool async = false);
std::string fetchFile(const std::string &path, const std::string &proxy, int cache_ttl);

#endif // MULTITHREAD_H_INCLUDED
