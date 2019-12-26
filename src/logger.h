#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <string>

#include "misc.h"

#define LOG_TYPE_INFO 1
#define LOG_TYPE_ERROR 2
#define LOG_TYPE_RAW 3
#define LOG_TYPE_WARN 4
#define LOG_TYPE_TCPING 5
#define LOG_TYPE_FILEDL 6
#define LOG_TYPE_GEOIP 7
#define LOG_TYPE_RULES 8
#define LOG_TYPE_GPING 9
#define LOG_TYPE_RENDER 10
#define LOG_TYPE_FILEUL 11

extern std::string resultPath, logPath;

int makeDir(const char *path);
std::string getTime(int type);
void logInit(bool rpcmode);
void resultInit();
void writeLog(int type, std::string content);
void logEOF();

/*
void resultInit(bool export_with_maxspeed);
void writeResult(nodeInfo *node, bool export_with_maxspeed);
void resultEOF(std::string traffic, int worknodes, int totnodes);
void exportResult(std::string outpath, std::string utiljspath, std::string stylepath, bool export_with_maxspeed);
*/
#endif // LOGGER_H_INCLUDED
