#include "collection_print.hpp"
#include "serializer.h"
#include "yaml-cpp/yaml.h"

#include <cassert>
#include <iostream>

int main()
{
    using namespace collection_print;

    try {
        const auto file = YAML::LoadFile("yaml/prototype.yaml");
        for (const auto& type : file["types"]) {
            std::cout << "for type with tag '" << type["tag"] << "'" << std::endl;
            for (std::size_t i{}; const auto& nested : type["nested"]) {
                std::cout << "    ";

                if (nested.IsScalar()) {
                    std::cout << "tag: " << nested.Scalar() << std::endl;
                }
                else if (nested.IsMap()) {
                    if (nested["regex"]) {
                        std::cout << "regex: " << nested["regex"] << std::endl;
                    }
                    else if (nested["dyn-regex"]) {
                        std::cout << "dyn-regex: " << nested["dyn-regex"] << std::endl;
                    }
                    else {
                        std::cout << "unknown map nested" << std::endl;
                    }
                }
                else {
                    std::cout << "unknown nested" << std::endl;
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}
