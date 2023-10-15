#ifndef REGEXP_H_INCLUDED
#define REGEXP_H_INCLUDED

#include <string>

bool regValid(const std::string &reg);
bool regFind(const std::string &src, const std::string &match);
std::string regReplace(const std::string &src, const std::string &match, const std::string &rep, bool global = true, bool multiline = true);
bool regMatch(const std::string &src, const std::string &match);
int regGetMatch(const std::string &src, const std::string &match, size_t group_count, ...);
std::vector<std::string> regGetAllMatch(const std::string &src, const std::string &match, bool group_only = false);
std::string regTrim(const std::string &src);

#endif // REGEXP_H_INCLUDED
