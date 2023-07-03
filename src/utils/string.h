#ifndef STRING_H_INCLUDED
#define STRING_H_INCLUDED

#include <numeric>
#include <string>
#include <sstream>
#include <vector>
#include <map>

using string = std::string;
using string_size = std::string::size_type;
using string_array = std::vector<std::string>;
using string_map = std::map<std::string, std::string>;
using string_pair_array = std::vector<std::pair<std::string, std::string>>;

std::vector<std::string> split(const std::string &s, const std::string &seperator);
std::string join(const string_array &arr, const std::string &delimiter);

template <typename InputIt>
std::string join(InputIt first, InputIt last, const std::string &delimiter)
{
    if(first == last)
        return "";
    if(std::next(first) == last)
        return *first;
    return std::accumulate(std::next(first), last, *first, [&](const std::string &a, const std::string &b) {return a + delimiter + b; });
}

std::string getUrlArg(const std::string &url, const std::string &request);
std::string replaceAllDistinct(std::string str, const std::string &old_value, const std::string &new_value);
std::string trimOf(const std::string& str, char target, bool before = true, bool after = true);
std::string trim(const std::string& str, bool before = true, bool after = true);
std::string trimQuote(const std::string &str, bool before = true, bool after = true);
void trimSelfOf(std::string &str, char target, bool before = true, bool after = true);
std::string trimWhitespace(const std::string &str, bool before = false, bool after = true);
std::string randomStr(const int len);
bool isStrUTF8(const std::string &data);

void removeUTF8BOM(std::string &data);
std::string UTF8ToCodePoint(const std::string &data);
std::string toLower(const std::string &str);
std::string toUpper(const std::string &str);
void processEscapeChar(std::string &str);
void processEscapeCharReverse(std::string &str);
int parseCommaKeyValue(const std::string &input, const std::string &separator, string_pair_array &result);

inline bool strFind(const std::string &str, const std::string &target)
{
    return str.find(target) != str.npos;
}

inline bool startsWith(const std::string &hay, const std::string &needle)
{
    return hay.substr(0, needle.length()) == needle;
}

inline bool endsWith(const std::string &hay, const std::string &needle)
{
    std::string::size_type hl = hay.length(), nl = needle.length();
    return hl >= nl && hay.substr(hl - nl, nl) == needle;
}

inline bool count_least(const std::string &hay, const char needle, size_t cnt)
{
    string_size pos = hay.find(needle);
    while(pos != hay.npos)
    {
        cnt--;
        if(!cnt)
            return true;
        pos = hay.find(needle, pos + 1);
    }
    return false;
}

inline char getLineBreak(const std::string &str)
{
    return count_least(str, '\n', 1) ? '\n' : '\r';
}

template <typename T, typename U> static inline T to_number(const U &value, T def_value = T())
{
    T retval = 0.0;
    char c;
    std::stringstream ss;
    ss << value;
    if(!(ss >> retval))
        return def_value;
    else if(ss >> c)
        return def_value;
    else
        return retval;
}

int to_int(const std::string &str, int def_value = 0);

#ifndef HAVE_TO_STRING
namespace std
{
template <typename T> std::string to_string(const T& n)
{
    std::ostringstream ss;
    ss << n;
    return ss.str();
}
}
#endif // HAVE_TO_STRING

#endif // STRING_H_INCLUDED
