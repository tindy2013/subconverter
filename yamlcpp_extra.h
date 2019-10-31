#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "misc.h"

YAML::Node vectorToNode(string_array data)
{
    YAML::Node node;
    for(std::string &x : data)
        node.push_back(x);
    return node;
}
