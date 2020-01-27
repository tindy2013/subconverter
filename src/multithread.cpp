#include <mutex>

#include "misc.h"

//safety lock for multi-thread
typedef std::lock_guard<std::mutex> guarded_mutex;
std::mutex on_configuring, on_emoji, on_rename;

extern string_array emojis, renames;

string_array safe_get_emojis()
{
    guarded_mutex guard(on_emoji);
    return emojis;
}

string_array safe_get_renames()
{
    guarded_mutex guard(on_rename);
    return renames;
}

void safe_set_emojis(string_array &data)
{
    guarded_mutex guard(on_emoji);
    emojis.swap(data);
}

void safe_set_renames(string_array &data)
{
    guarded_mutex guard(on_rename);
    renames.swap(data);
}
