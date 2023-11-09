#include <string>

#include "string.h"

unsigned char toHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

unsigned char fromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z')
        y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z')
        y = x - 'a' + 10;
    else if (x >= '0' && x <= '9')
        y = x - '0';
    else
        y = x;
    return y;
}

std::string urlEncode(const std::string& str)
{
    std::string strTemp = "";
    string_size length = str.length();
    for (string_size i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
                (str[i] == '-') ||
                (str[i] == '_') ||
                (str[i] == '.') ||
                (str[i] == '~'))
            strTemp += str[i];
        else
        {
            strTemp += '%';
            strTemp += toHex((unsigned char)str[i] >> 4);
            strTemp += toHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

std::string urlDecode(const std::string& str)
{
    std::string strTemp;
    string_size length = str.length();
    for (string_size i = 0; i < length; i++)
    {
        if (str[i] == '+')
            strTemp += ' ';
        else if (str[i] == '%')
        {
            if(i + 2 >= length)
                return strTemp;
            if(isalnum(str[i + 1]) && isalnum(str[i + 2]))
            {
                unsigned char high = fromHex((unsigned char)str[++i]);
                unsigned char low = fromHex((unsigned char)str[++i]);
                strTemp += high * 16 + low;
            }
            else
                strTemp += str[i];
        }
        else
            strTemp += str[i];
    }
    return strTemp;
}

std::string joinArguments(const string_multimap &args)
{
    std::string strTemp;
    for (auto &p: args)
    {
        strTemp += p.first + "=" + urlEncode(p.second) + "&";
    }
    if (!strTemp.empty())
    {
        strTemp.pop_back();
    }
    return strTemp;
}
