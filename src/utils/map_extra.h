#ifndef MAP_EXTRA_H_INCLUDED
#define MAP_EXTRA_H_INCLUDED

#include <string>
#include <map>
#include <algorithm>

struct strICaseComp
{
    bool operator() (const std::string &lhs, const std::string &rhs) const
    {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
                                            rhs.end(),
                                            [](unsigned char c1, unsigned char c2)
                                            {
                                                return ::tolower(c1) < ::tolower(c2);
                                            });
    }
};

using string_icase_map = std::map<std::string, std::string, strICaseComp>;

#endif // MAP_EXTRA_H_INCLUDED
