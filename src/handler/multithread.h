#ifndef MULTITHREAD_H_INCLUDED
#define MULTITHREAD_H_INCLUDED

#include <mutex>
#include <future>

#include <yaml-cpp/yaml.h>

#include "config/regmatch.h"
#include "utils/ini_reader/ini_reader.h"
#include "utils/string.h"

using guarded_mutex = std::lock_guard<std::mutex>;

RegexMatchConfigs safe_get_emojis();
RegexMatchConfigs safe_get_renames();
RegexMatchConfigs safe_get_streams();
RegexMatchConfigs safe_get_times();
YAML::Node safe_get_clash_base();
INIReader safe_get_mellow_base();
void safe_set_emojis(RegexMatchConfigs data);
void safe_set_renames(RegexMatchConfigs data);
void safe_set_streams(RegexMatchConfigs data);
void safe_set_times(RegexMatchConfigs data);
std::shared_future<std::string> fetchFileAsync(const std::string &path, const std::string &proxy, int cache_ttl, bool find_local = true, bool async = false);
std::string fetchFile(const std::string &path, const std::string &proxy, int cache_ttl, bool find_local = true);

#endif // MULTITHREAD_H_INCLUDED
