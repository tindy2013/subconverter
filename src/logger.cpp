#include <string>
#include <iostream>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "logger.h"

extern bool print_debug_info;
int global_log_level = LOG_LEVEL_INFO;

std::string getTime(int type)
{
    time_t lt;
    char tmpbuf[32], cMillis[7];
    std::string format;
    timeval tv;
    gettimeofday(&tv, NULL);
    snprintf(cMillis, 7, "%.6ld", (long)tv.tv_usec);
    lt = time(NULL);
    struct tm *local = localtime(&lt);
    switch(type)
    {
    case 1:
        format = "%Y%m%d-%H%M%S";
        break;
    case 2:
        format = "%Y/%m/%d %a %H:%M:%S." + std::string(cMillis);
        break;
    case 3:
        format = "%Y-%m-%d %H:%M:%S";
        break;
    }
    strftime(tmpbuf, 32, format.data(), local);
    return std::string(tmpbuf);
}

void writeLog(int type, std::string content, int level)
{
    //placeholder
    /*
    if(print_debug_info)
        std::cerr<<getTime(2)<<" [DEBUG] "<<content<<"\n";
    */
    if(level > global_log_level)
        return;
    //std::cerr<<getTime(2)<<" ["<<getpid()<<"]";

    switch(level)
    {
    case LOG_LEVEL_VERBOSE:
        std::cerr<<getTime(2)<<" ["<<getpid()<<"]"<<"[VERB]"<<" "<<content<<"\n";
        break;
    case LOG_LEVEL_DEBUG:
        std::cerr<<getTime(2)<<" ["<<getpid()<<"]"<<"[DEBG]"<<" "<<content<<"\n";
        break;
    case LOG_LEVEL_INFO:
        std::cerr<<getTime(2)<<" ["<<getpid()<<"]"<<"[INFO]"<<" "<<content<<"\n";
        break;
    case LOG_LEVEL_WARNING:
        std::cerr<<getTime(2)<<" ["<<getpid()<<"]"<<"[WARN]"<<" "<<content<<"\n";
        break;
    case LOG_LEVEL_ERROR:
        std::cerr<<getTime(2)<<" ["<<getpid()<<"]"<<"[ERRO]"<<" "<<content<<"\n";
        break;
    case LOG_LEVEL_FATAL:
        std::cerr<<getTime(2)<<" ["<<getpid()<<"]"<<"[FATL]"<<" "<<content<<"\n";
        break;
    }
    //std::cerr<<" "<<content<<"\n";
}
