#include <string>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "logger.h"

extern bool gPrintDbgInfo;
int gLogLevel = LOG_LEVEL_INFO;

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

void writeLog(int type, const std::string &content, int level)
{
    if(level > gLogLevel)
        return;
    const char *levels[] = {"[FATL]", "[ERRO]", "[WARN]", "[INFO]", "[DEBG]", "[VERB]"};
    std::cerr<<getTime(2)<<" ["<<getpid()<<" "<<std::this_thread::get_id()<<"]"<<levels[level % 6];
    std::cerr<<" "<<content<<"\n";
}
