#ifndef CRONTASK_H_INCLUDED
#define CRONTASK_H_INCLUDED

#include "def.h"

struct CronTaskConfig
{
    String Name;
    String CronExp;
    String Path;
    Integer Timeout = 0;
};

using CronTaskConfigs = std::vector<CronTaskConfig>;

#endif // CRONTASK_H_INCLUDED
