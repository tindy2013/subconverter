#include <mutex>

#include "misc.h"

//safety lock for multi-thread
typedef std::lock_guard<std::mutex> guarded_mutex;
std::mutex on_configuring, on_emoji, on_rename, on_stream, on_time;

extern string_array emojis, renames;
extern string_array stream_rules, time_rules;

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

string_array safe_get_streams()
{
    guarded_mutex guard(on_stream);
    return stream_rules;
}

string_array safe_get_times()
{
    guarded_mutex guard(on_time);
    return time_rules;
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

void safe_set_streams(string_array &data)
{
    guarded_mutex guard(on_stream);
    stream_rules.swap(data);
}

void safe_set_times(string_array &data)
{
    guarded_mutex guard(on_time);
    time_rules.swap(data);
}
