#ifndef RAPIDJSON_EXTRA_H_INCLUDED
#define RAPIDJSON_EXTRA_H_INCLUDED

#include <rapidjson/document.h>
#include <string>

void operator >> (const rapidjson::Value& value, std::string& i);
void operator >> (const rapidjson::Value& value, int& i);
std::string GetMember(const rapidjson::Value& value, const std::string &member);
void GetMember(const rapidjson::Value& value, const std::string &member, std::string& target);
std::string SerializeObject(const rapidjson::Value& value);

#endif // RAPIDJSON_EXTRA_H_INCLUDED
