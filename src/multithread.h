#ifndef MULTITHREAD_H_INCLUDED
#define MULTITHREAD_H_INCLUDED

#include <mutex>
#include "misc.h"

typedef std::lock_guard<std::mutex> guarded_mutex;

string_array safe_get_emojis();
string_array safe_get_renames();
string_array safe_get_streams();
string_array safe_get_times();
void safe_set_emojis(string_array &data);
void safe_set_renames(string_array &data);
void safe_set_streams(string_array &data);
void safe_set_times(string_array &data);

#endif // MULTITHREAD_H_INCLUDED
