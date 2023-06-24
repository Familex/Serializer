#pragma once

#include "structs/properties.hpp"

#include <string>

namespace dynser::util
{

Properties add_prefix(Properties&& props, std::string_view prefix) noexcept
{
    Properties result;
    for (auto& [key, value] : props) {
        result[prefix.data() + ('-' + key)] = std::move(value);
    }
    return result;
}

Properties remove_prefix(Properties& props, std::string_view prefix) noexcept
{
    Properties result;
    for (auto& [key, value] : props) {
        if (key.starts_with(prefix)) {
            result.insert({ key.substr(prefix.size()), value });
        }
    }
    return result;
}

}    // namespace dynser::util
