#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#include <string>

void sleepMs(int interval);
std::string getEnv(const std::string &name);
std::string getSystemProxy();

#endif // SYSTEM_H_INCLUDED
