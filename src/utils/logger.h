#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <string>
#include <typeinfo>

enum
{
    LOG_TYPE_INFO,
    LOG_TYPE_ERROR,
    LOG_TYPE_RAW,
    LOG_TYPE_WARN,
    LOG_TYPE_TCPING,
    LOG_TYPE_FILEDL,
    LOG_TYPE_GEOIP,
    LOG_TYPE_RULES,
    LOG_TYPE_GPING,
    LOG_TYPE_RENDER,
    LOG_TYPE_FILEUL
};

enum
{
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_VERBOSE
};

std::string getTime(int type);
void writeLog(int type, const std::string &content, int level = LOG_LEVEL_VERBOSE);
std::string demangle(const char* name);

template <class T>
std::string type(const T& t) {

    return demangle(typeid(t).name());
}

#endif // LOGGER_H_INCLUDED
