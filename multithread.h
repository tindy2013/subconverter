#ifndef MULTITHREAD_H_INCLUDED
#define MULTITHREAD_H_INCLUDED

#include <mutex>
#include "misc.h"

typedef std::lock_guard<std::mutex> guarded_mutex;

void try_config_lock();
void try_emoji_lock();
void try_rename_lock();
string_array safe_get_emojis();
string_array safe_get_renames();
void safe_set_emojis(string_array &data);
void safe_set_renames(string_array &data);

#endif // MULTITHREAD_H_INCLUDED
