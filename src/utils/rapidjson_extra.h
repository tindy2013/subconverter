#ifndef RAPIDJSON_EXTRA_H_INCLUDED
#define RAPIDJSON_EXTRA_H_INCLUDED

#include <stdexcept>

template <typename T> void exception_thrower(T e, const std::string &cond, const std::string &file, int line)
{
    if(!e)
        throw std::runtime_error("rapidjson assertion failed: " + cond + " (" + file + ":" + std::to_string(line) + ")");
}

#ifdef RAPIDJSON_ASSERT
#undef RAPIDJSON_ASSERT
#endif // RAPIDJSON_ASSERT
#define VALUE(x) #x
#define RAPIDJSON_ASSERT(x) exception_thrower(x, VALUE(x), __FILE__, __LINE__)
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>
#include <string>

inline void operator >> (const rapidjson::Value& value, std::string& i)
{
    if(value.IsNull())
        i = "";
    else if(value.IsString())
        i = std::string(value.GetString());
    else if(value.IsInt64())
        i = std::to_string(value.GetInt64());
    else if(value.IsBool())
        i = value.GetBool() ? "true" : "false";
    else if(value.IsDouble())
        i = std::to_string(value.GetDouble());
    else
        i = "";
}

inline void operator >> (const rapidjson::Value& value, int& i)
{
    if(value.IsNull())
        i = 0;
    else if(value.IsInt())
        i = value.GetInt();
    else if(value.IsString())
        i = std::stoi(value.GetString());
    else if(value.IsBool())
        i = value.GetBool() ? 1 : 0;
    else
        i = 0;
}

inline std::string GetMember(const rapidjson::Value& value, const std::string &member)
{
    std::string retStr;
    if(value.IsObject() && value.HasMember(member.data()))
        value[member.data()] >> retStr;
    return retStr;
}

inline void GetMember(const rapidjson::Value& value, const std::string &member, std::string& target)
{
    std::string retStr = GetMember(value, member);
    if(retStr.size())
        target.assign(retStr);
}

inline std::string SerializeObject(const rapidjson::Value& value)
{
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer_json(sb);
    value.Accept(writer_json);
    return sb.GetString();
}


#endif // RAPIDJSON_EXTRA_H_INCLUDED
