#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <string>

#include "misc.h"

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

extern std::string resultPath, logPath;

int makeDir(const char *path);
std::string getTime(int type);
void logInit(bool rpcmode);
void resultInit();
void writeLog(int type, std::string content, int level = LOG_LEVEL_VERBOSE);
void logEOF();

/*
void resultInit(bool export_with_maxspeed);
void writeResult(nodeInfo *node, bool export_with_maxspeed);
void resultEOF(std::string traffic, int worknodes, int totnodes);
void exportResult(std::string outpath, std::string utiljspath, std::string stylepath, bool export_with_maxspeed);
*/
#endif // LOGGER_H_INCLUDED
