#include <string>
#include <cstdarg>

/*
#ifdef USE_STD_REGEX
#include <regex>
#else
*/
#include <jpcre2.hpp>
using jp = jpcre2::select<char>;
//#endif // USE_STD_REGEX

#include "regexp.h"

/*
#ifdef USE_STD_REGEX
bool regValid(const std::string &reg)
{
    try
    {
        std::regex r(reg, std::regex::ECMAScript);
        return true;
    }
    catch (std::regex_error &e)
    {
        return false;
    }
}

bool regFind(const std::string &src, const std::string &match)
{
    try
    {
        std::regex::flag_type flags = std::regex::extended | std::regex::ECMAScript;
        std::string target = match;
        if(match.find("(?i)") == 0)
        {
            target.erase(0, 4);
            flags |= std::regex::icase;
        }
        std::regex reg(target, flags);
        return regex_search(src, reg);
    }
    catch (std::regex_error &e)
    {
        return false;
    }
}

std::string regReplace(const std::string &src, const std::string &match, const std::string &rep)
{
    std::string result = "";
    try
    {
        std::regex::flag_type flags = std::regex::extended | std::regex::ECMAScript;
        std::string target = match;
        if(match.find("(?i)") == 0)
        {
            target.erase(0, 4);
            flags |= std::regex::icase;
        }
        std::regex reg(target, flags);
        regex_replace(back_inserter(result), src.begin(), src.end(), reg, rep);
    }
    catch (std::regex_error &e)
    {
        result = src;
    }
    return result;
}

bool regMatch(const std::string &src, const std::string &match)
{
    try
    {
        std::regex::flag_type flags = std::regex::extended | std::regex::ECMAScript;
        std::string target = match;
        if(match.find("(?i)") == 0)
        {
            target.erase(0, 4);
            flags |= std::regex::icase;
        }
        std::regex reg(target, flags);
        return regex_match(src, reg);
    }
    catch (std::regex_error &e)
    {
        return false;
    }
}

int regGetMatch(const std::string &src, const std::string &match, size_t group_count, ...)
{
    try
    {
        std::regex::flag_type flags = std::regex::extended | std::regex::ECMAScript;
        std::string target = match;
        if(match.find("(?i)") == 0)
        {
            target.erase(0, 4);
            flags |= std::regex::icase;
        }
        std::regex reg(target, flags);
        std::smatch result;
        if(regex_search(src.cbegin(), src.cend(), result, reg))
        {
            if(result.size() < group_count - 1)
                return -1;
            va_list vl;
            va_start(vl, group_count);
            size_t index = 0;
            while(group_count)
            {
                std::string* arg = va_arg(vl, std::string*);
                if(arg != NULL)
                    *arg = std::move(result[index]);
                index++;
                group_count--;
            }
            va_end(vl);
        }
        else
            return -2;
        return 0;
    }
    catch (std::regex_error&)
    {
        return -3;
    }
}

#else
*/
bool regMatch(const std::string &src, const std::string &match)
{
    jp::Regex reg;
    reg.setPattern(match).addModifier("m").addPcre2Option(PCRE2_ANCHORED|PCRE2_ENDANCHORED|PCRE2_UTF).compile();
    if(!reg)
        return false;
    return reg.match(src, "g");
}

bool regFind(const std::string &src, const std::string &match)
{
    jp::Regex reg;
    reg.setPattern(match).addModifier("m").addPcre2Option(PCRE2_UTF|PCRE2_ALT_BSUX).compile();
    if(!reg)
        return false;
    return reg.match(src, "g");
}

std::string regReplace(const std::string &src, const std::string &match, const std::string &rep, bool global, bool multiline)
{
    jp::Regex reg;
    reg.setPattern(match).addModifier(multiline ? "m" : "").addPcre2Option(PCRE2_UTF|PCRE2_MULTILINE|PCRE2_ALT_BSUX).compile();
    if(!reg)
        return src;
    return reg.replace(src, rep, global ? "gEx" : "Ex");
}

bool regValid(const std::string &reg)
{
    jp::Regex r;
    r.setPattern(reg).addPcre2Option(PCRE2_UTF|PCRE2_ALT_BSUX).compile();
    return !!r;
}

int regGetMatch(const std::string &src, const std::string &match, size_t group_count, ...)
{
    auto result = regGetAllMatch(src, match, false);
    if(result.empty())
        return -1;
    va_list vl;
    va_start(vl, group_count);
    size_t index = 0;
    while(group_count)
    {
        std::string* arg = va_arg(vl, std::string*);
        if(arg != nullptr)
            *arg = std::move(result[index]);
        index++;
        group_count--;
        if(result.size() <= index)
            break;
    }
    va_end(vl);
    return 0;
}

std::vector<std::string> regGetAllMatch(const std::string &src, const std::string &match, bool group_only)
{
    jp::Regex reg;
    reg.setPattern(match).addModifier("m").addPcre2Option(PCRE2_UTF|PCRE2_ALT_BSUX).compile();
    jp::VecNum vec_num;
    jp::RegexMatch rm;
    size_t count = rm.setRegexObject(&reg).setSubject(src).setNumberedSubstringVector(&vec_num).setModifier("gm").match();
    std::vector<std::string> result;
    if(!count)
        return result;
    size_t begin = 0;
    if(group_only)
        begin = 1;
    size_t index = begin, match_index = 0;
    while(true)
    {
        if(vec_num.size() <= match_index)
            break;
        if(vec_num[match_index].size() <= index)
        {
            match_index++;
            index = begin;
            continue;
        }
        result.push_back(std::move(vec_num[match_index][index]));
        index++;
    }
    return result;
}

//#endif // USE_STD_REGEX

std::string regTrim(const std::string &src)
{
    return regReplace(src, R"(^\s*([\s\S]*)\s*$)", "$1", false, false);
}
