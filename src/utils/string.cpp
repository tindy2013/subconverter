#include <algorithm>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <time.h>

#include "string.h"

std::vector<std::string> split(const std::string &s, const std::string &seperator)
{
    std::vector<std::string> result;
    string_size i = 0;

    while(i != s.size())
    {
        int flag = 0;
        while(i != s.size() && flag == 0)
        {
            flag = 1;
            for(string_size x = 0; x < seperator.size(); ++x)
                if(s[i] == seperator[x])
                {
                    ++i;
                    flag = 0;
                    break;
                }
        }

        flag = 0;
        string_size j = i;
        while(j != s.size() && flag == 0)
        {
            for(string_size x = 0; x < seperator.size(); ++x)
                if(s[j] == seperator[x])
                {
                    flag = 1;
                    break;
                }
            if(flag == 0)
                ++j;
        }
        if(i != j)
        {
            result.push_back(s.substr(i, j-i));
            i = j;
        }
    }
    return result;
}

std::string UTF8ToCodePoint(const std::string &data)
{
    std::stringstream ss;
    for(string_size i = 0; i < data.size(); i++)
    {
        int charcode = data[i] & 0xff;
        if((charcode >> 7) == 0)
        {
            ss<<data[i];
        }
        else if((charcode >> 5) == 6)
        {
            ss<<"\\u"<<std::hex<<((data[i + 1] & 0x3f) | (data[i] & 0x1f) << 6);
            i++;
        }
        else if((charcode >> 4) == 14)
        {
            ss<<"\\u"<<std::hex<<((data[i + 2] & 0x3f) | (data[i + 1] & 0x3f) << 6 | (data[i] & 0xf) << 12);
            i += 2;
        }
        else if((charcode >> 3) == 30)
        {
            ss<<"\\u"<<std::hex<<((data[i + 3] & 0x3f) | (data[i + 2] & 0x3f) << 6 | (data[i + 1] & 0x3f) << 12 | (data[i] & 0x7) << 18);
            i += 3;
        }
    }
    return ss.str();
}

std::string toLower(const std::string &str)
{
    std::string result;
    std::transform(str.begin(), str.end(), std::back_inserter(result), [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string toUpper(const std::string &str)
{
    std::string result;
    std::transform(str.begin(), str.end(), std::back_inserter(result), [](unsigned char c) { return std::toupper(c); });
    return result;
}

void processEscapeChar(std::string &str)
{
    string_size pos = str.find('\\');
    while(pos != str.npos)
    {
        if(pos == str.size())
            break;
        switch(str[pos + 1])
        {
        case 'n':
            str.replace(pos, 2, "\n");
            break;
        case 'r':
            str.replace(pos, 2, "\r");
            break;
        case 't':
            str.replace(pos, 2, "\t");
            break;
        default:
            /// ignore others for backward compatibility
            //str.erase(pos, 1);
            break;
        }
        pos = str.find('\\', pos + 1);
    }
}

void processEscapeCharReverse(std::string &str)
{
    string_size pos = 0;
    while(pos < str.size())
    {
        switch(str[pos])
        {
        case '\n':
            str.replace(pos, 1, "\\n");
            break;
        case '\r':
            str.replace(pos, 1, "\\r");
            break;
        case '\t':
            str.replace(pos, 1, "\\t");
            break;
        default:
            /// ignore others for backward compatibility
            break;
        }
        pos++;
    }
}

int parseCommaKeyValue(const std::string &input, const std::string &separator, string_pair_array &result)
{
    string_size bpos = 0, epos = input.find(',');
    std::string kv;
    while(bpos < input.size())
    {
        if(epos == std::string::npos)
            epos = input.size();
        else if(epos && input[epos - 1] == '\\')
        {
            kv += input.substr(bpos, epos - bpos - 1);
            kv += ',';
            bpos = epos + 1;
            epos = input.find(',', bpos);
            continue;
        }
        kv += input.substr(bpos, epos - bpos);
        string_size eqpos = kv.find('=');
        if(eqpos == std::string::npos)
            result.emplace_back("{NONAME}", kv);
        else
            result.emplace_back(kv.substr(0, eqpos), kv.substr(eqpos + 1));
        kv.clear();
        bpos = epos + 1;
        epos = input.find(',', bpos);
    }
    if(kv.size())
    {
        string_size eqpos = kv.find('=');
        if(eqpos == std::string::npos)
            result.emplace_back("{NONAME}", kv);
        else
            result.emplace_back(kv.substr(0, eqpos), kv.substr(eqpos + 1));
    }
    return 0;
}

void trimSelfOf(std::string &str, char target, bool before, bool after)
{
    if (!before && !after)
        return;
    std::string::size_type pos = str.size() - 1;
    if (after)
        pos = str.find_last_not_of(target);
    if (pos != std::string::npos)
        str.erase(pos + 1);
    if (before)
        pos = str.find_first_not_of(target);
    str.erase(0, pos);
}

std::string trimOf(const std::string& str, char target, bool before, bool after)
{
    if (!before && !after)
        return str;
    std::string::size_type pos = 0;
    if (before)
        pos = str.find_first_not_of(target);
    if (pos == std::string::npos)
    {
        return str;
    }
    std::string::size_type pos2 = str.size() - 1;
    if (after)
        pos2 = str.find_last_not_of(target);
    if (pos2 != std::string::npos)
    {
        return str.substr(pos, pos2 - pos + 1);
    }
    return str.substr(pos);
}

std::string trim(const std::string& str, bool before, bool after)
{
    return trimOf(str, ' ', before, after);
}

std::string trimQuote(const std::string &str, bool before, bool after)
{
    return trimOf(str, '\"', before, after);
}

std::string trimWhitespace(const std::string &str, bool before, bool after)
{
    static std::string whitespaces(" \t\f\v\n\r");
    string_size bpos = 0, epos = str.size();
    if(after)
    {
        epos = str.find_last_not_of(whitespaces);
        if(epos == std::string::npos)
            return "";
    }
    if(before)
    {
        bpos = str.find_first_not_of(whitespaces);
        if(bpos == std::string::npos)
            return "";
    }
    return str.substr(bpos, epos - bpos + 1);
}

std::string getUrlArg(const std::string &url, const std::string &request)
{
    //std::smatch result;
    /*
    if (regex_search(url.cbegin(), url.cend(), result, std::regex(request + "=(.*?)&")))
    {
        return result[1];
    }
    else if (regex_search(url.cbegin(), url.cend(), result, std::regex(request + "=(.*)")))
    {
        return result[1];
    }
    else
    {
        return std::string();
    }
    */
    /*
    std::string::size_type spos = url.find("?");
    if(spos != url.npos)
        url.erase(0, spos + 1);

    string_array vArray, arglist = split(url, "&");
    for(std::string &x : arglist)
    {
        std::string::size_type epos = x.find("=");
        if(epos != x.npos)
        {
            if(x.substr(0, epos) == request)
                return x.substr(epos + 1);
        }
    }
    */
    std::string pattern = request + "=";
    std::string::size_type pos = url.size();
    while(pos)
    {
        pos = url.rfind(pattern, pos);
        if(pos != url.npos)
        {
            if(pos == 0 || url[pos - 1] == '&' || url[pos - 1] == '?')
            {
                pos += pattern.size();
                return url.substr(pos, url.find("&", pos) - pos);
            }
        }
        else
            break;
        pos--;
    }
    return "";
}

std::string replaceAllDistinct(std::string str, const std::string &old_value, const std::string &new_value)
{
    for(std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length())
    {
        if((pos = str.find(old_value, pos)) != std::string::npos)
            str.replace(pos, old_value.length(), new_value);
        else
            break;
    }
    return str;
}

void removeUTF8BOM(std::string &data)
{
    if(data.compare(0, 3, "\xEF\xBB\xBF") == 0)
        data = data.substr(3);
}

bool isStrUTF8(const std::string &data)
{
    const char *str = data.c_str();
    unsigned int nBytes = 0;
    for (unsigned int i = 0; str[i] != '\0'; ++i)
    {
        unsigned char chr = *(str + i);
        if (nBytes == 0)
        {
            if (chr >= 0x80)
            {
                if (chr >= 0xFC && chr <= 0xFD)
                    nBytes = 6;
                else if (chr >= 0xF8)
                    nBytes = 5;
                else if (chr >= 0xF0)
                    nBytes = 4;
                else if (chr >= 0xE0)
                    nBytes = 3;
                else if (chr >= 0xC0)
                    nBytes = 2;
                else
                    return false;
                nBytes--;
            }
        }
        else
        {
            if ((chr & 0xC0) != 0x80)
                return false;
            nBytes--;
        }
    }
    if (nBytes != 0)
        return false;
    return true;
}

std::string randomStr(const int len)
{
    std::string retData;
    srand(time(NULL));
    int cnt = 0;
    while(cnt < len)
    {
        switch((rand() % 3))
        {
        case 1:
            retData += ('A' + rand() % 26);
            break;
        case 2:
            retData += ('a' + rand() % 26);
            break;
        default:
            retData += ('0' + rand() % 10);
            break;
        }
        cnt++;
    }
    return retData;
}

int to_int(const std::string &str, int def_value)
{
    if(str.empty())
        return def_value;
    /*
    int retval = 0;
    char c;
    std::stringstream ss(str);
    if(!(ss >> retval))
        return def_value;
    else if(ss >> c)
        return def_value;
    else
        return retval;
    */
    return std::atoi(str.data());
}

std::string join(const string_array &arr, const std::string &delimiter)
{
    if(arr.size() == 0)
        return "";
    if(arr.size() == 1)
        return arr[0];
    return std::accumulate(arr.begin() + 1, arr.end(), arr[0], [&](const std::string &a, const std::string &b) {return a + delimiter + b; });
}
