#ifndef YAMLCPP_EXTRA_H_INCLUDED
#define YAMLCPP_EXTRA_H_INCLUDED

#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>

template <typename T> void operator >> (const YAML::Node& node, T& i)
{
    if(node.IsDefined() && !node.IsNull()) //fail-safe
        i = node.as<T>();
};

template <typename T> T safe_as (const YAML::Node& node)
{
    if(node.IsDefined() && !node.IsNull())
        return node.as<T>();
    return T();
};

template <typename T> void operator >>= (const YAML::Node& node, T& i)
{
    i = safe_as<T>(node);
};

using string_array = std::vector<std::string>;

inline std::string dump_to_pairs (const YAML::Node &node, const string_array &exclude = string_array())
{
    std::string result;
    for(auto iter = node.begin(); iter != node.end(); iter++)
    {
        if(iter->second.Type() != YAML::NodeType::Scalar)
            continue;
        std::string key = iter->first.as<std::string>();
        if(std::find(exclude.cbegin(), exclude.cend(), key) != exclude.cend())
            continue;
        std::string value = iter->second.as<std::string>();
        result += key + "=" + value + ",";
    }
    return result.erase(result.size() - 1);
}

#endif // YAMLCPP_EXTRA_H_INCLUDED
