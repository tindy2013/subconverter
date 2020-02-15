#include "multithread.h"

//safety lock for multi-thread
std::mutex on_emoji, on_rename, on_stream, on_time, clash_base_mutex, mellow_base_mutex;

extern string_array emojis, renames;
extern string_array stream_rules, time_rules;
extern YAML::Node clash_base;
extern INIReader mellow_base;

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

YAML::Node safe_get_clash_base()
{
    guarded_mutex guard(clash_base_mutex);
    return clash_base;
}

INIReader safe_get_mellow_base()
{
    guarded_mutex guard(mellow_base_mutex);
    return mellow_base;
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
