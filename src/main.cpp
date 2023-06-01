#include "serializer.h"
#include "yaml-cpp/yaml.h"

#include <cassert>
#include <iostream>

int main()
{
    // example from yaml-cpp wiki

    YAML::Node node = YAML::Load("{name: Brewers, city: Milwaukee}");
    if (node["name"]) {
        std::cout << node["name"].as<std::string>() << "\n";
    }
    if (node["mascot"]) {
        std::cout << node["mascot"].as<std::string>() << "\n";
    }
    assert(node.size() == 2);    // the previous call didn't create a node
}
