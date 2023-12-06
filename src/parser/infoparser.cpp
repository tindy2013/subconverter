#include <string>
#include <vector>
#include <cmath>
#include <ctime>

#include "config/regmatch.h"
#include "parser/config/proxy.h"
#include "utils/base64/base64.h"
#include "utils/rapidjson_extra.h"
#include "utils/regexp.h"
#include "utils/string.h"

unsigned long long streamToInt(const std::string &stream)
{
    if(stream.empty())
        return 0;
    double streamval = 1.0;
    std::vector<std::string> units = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
    size_t index = units.size();
    do
    {
        index--;
        if(endsWith(stream, units[index]))
        {
            streamval = std::pow(1024, index) * to_number<float>(stream.substr(0, stream.size() - units[index].size()), 0.0);
            break;
        }
    }
    while(index != 0);
    return (unsigned long long)streamval;
}

static inline double percentToDouble(const std::string &percent)
{
    return stof(percent.substr(0, percent.size() - 1)) / 100.0;
}

time_t dateStringToTimestamp(std::string date)
{
    time_t rawtime;
    time(&rawtime);
    if(startsWith(date, "left="))
    {
        time_t seconds_left = 0;
        date.erase(0, 5);
        if(endsWith(date, "d"))
        {
            date.erase(date.size() - 1);
            seconds_left = to_number<double>(date, 0.0) * 86400.0;
        }
        return rawtime + seconds_left;
    }
    else
    {
        struct tm *expire_time;
        std::vector<std::string> date_array = split(date, ":");
        if(date_array.size() != 6)
            return 0;

        expire_time = localtime(&rawtime);
        expire_time->tm_year = to_int(date_array[0], 1900) - 1900;
        expire_time->tm_mon = to_int(date_array[1], 1) - 1;
        expire_time->tm_mday = to_int(date_array[2]);
        expire_time->tm_hour = to_int(date_array[3]);
        expire_time->tm_min = to_int(date_array[4]);
        expire_time->tm_sec = to_int(date_array[5]);
        return mktime(expire_time);
    }
}

bool getSubInfoFromHeader(const std::string &header, std::string &result)
{
    std::string pattern = R"(^(?i:Subscription-UserInfo): (.*?)\s*?$)", retStr;
    if(regFind(header, pattern))
    {
        regGetMatch(header, pattern, 2, 0, &retStr);
        if(!retStr.empty())
        {
            result = retStr;
            return true;
        }
    }
    return false;
}

bool getSubInfoFromNodes(const std::vector<Proxy> &nodes, const RegexMatchConfigs &stream_rules, const RegexMatchConfigs &time_rules, std::string &result)
{
    std::string remarks, stream_info, time_info, retStr;

    for(const Proxy &x : nodes)
    {
        remarks = x.Remark;
        if(stream_info.empty())
        {
            for(const RegexMatchConfig &y : stream_rules)
            {
                if(regMatch(remarks, y.Match))
                {
                    retStr = regReplace(remarks, y.Match, y.Replace);
                    if(retStr != remarks)
                    {
                        stream_info = retStr;
                        break;
                    }
                }
                else
                    continue;
            }
        }

        remarks = x.Remark;
        if(time_info.empty())
        {
            for(const RegexMatchConfig &y : time_rules)
            {
                if(regMatch(remarks, y.Match))
                {
                    retStr = regReplace(remarks, y.Match, y.Replace);
                    if(retStr != remarks)
                    {
                        time_info = retStr;
                        break;
                    }
                }
                else
                    continue;
            }
        }

        if(!stream_info.empty() && !time_info.empty())
            break;
    }

    if(stream_info.empty() && time_info.empty())
        return false;

    //calculate how much stream left
    unsigned long long total = 0, left, used = 0, expire = 0;
    std::string total_str = getUrlArg(stream_info, "total"), left_str = getUrlArg(stream_info, "left"), used_str = getUrlArg(stream_info, "used");
    if(strFind(total_str, "%"))
    {
        if(!used_str.empty())
        {
            used = streamToInt(used_str);
            total = used / (1 - percentToDouble(total_str));
        }
        else if(!left_str.empty())
        {
            left = streamToInt(left_str);
            total = left / percentToDouble(total_str);
            if (left > total) left = 0;
            used = total - left;
        }
    }
    else
    {
        total = streamToInt(total_str);
        if(!used_str.empty())
        {
            used = streamToInt(used_str);
        }
        else if(!left_str.empty())
        {
            left = streamToInt(left_str);
            if (left > total) left = 0;
            used = total - left;
        }
    }

    result = "upload=0; download=" + std::to_string(used) + "; total=" + std::to_string(total) + ";";

    //calculate expire time
    expire = dateStringToTimestamp(time_info);
    if(expire)
        result += " expire=" + std::to_string(expire) + ";";

    return true;
}

bool getSubInfoFromSSD(const std::string &sub, std::string &result)
{
    rapidjson::Document json;
    json.Parse(urlSafeBase64Decode(sub.substr(6)).data());
    if(json.HasParseError())
        return false;

    std::string used_str = GetMember(json, "traffic_used"), total_str = GetMember(json, "traffic_total"), expire_str = GetMember(json, "expiry");
    if(used_str.empty() || total_str.empty())
        return false;
    unsigned long long used = stod(used_str) * std::pow(1024, 3), total = stod(total_str) * std::pow(1024, 3), expire;
    result = "upload=0; download=" + std::to_string(used) + "; total=" + std::to_string(total) + ";";

    expire = dateStringToTimestamp(regReplace(expire_str, "(\\d+)-(\\d+)-(\\d+) (.*)", "$1:$2:$3:$4"));
    if(expire)
        result += " expire=" + std::to_string(expire) + ";";

    return true;
}
