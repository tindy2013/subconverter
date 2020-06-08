#include <yaml-cpp/yaml.h>

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
