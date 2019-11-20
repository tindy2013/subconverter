#include <string>
#include <iostream>

#include "logger.h"

extern bool print_debug_info;

void writeLog(int type, std::string content)
{
    //placeholder
    if(print_debug_info)
        std::cerr<<"[DEBUG] "<<content<<"\n";
}
