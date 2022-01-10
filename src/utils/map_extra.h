#ifndef MAP_EXTRA_H_INCLUDED
#define MAP_EXTRA_H_INCLUDED

#include <string>
#include <map>
#include <string.h>

struct strICaseComp
{
    bool operator() (const std::string &lhs, const std::string &rhs) const
    {
        return strcasecmp(lhs.c_str(), rhs.c_str());
    }
};

using string_icase_map = std::map<std::string, std::string, strICaseComp>;

#endif // MAP_EXTRA_H_INCLUDED
