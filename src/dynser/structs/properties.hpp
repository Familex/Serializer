#pragma once

#include "luwra.hpp"

#include <any>
#include <map>
#include <string>

namespace dynser
{

/**
 * \brief Property value type, can be used in lua.
 */
struct PropertyValue
{
    std::any data;

    inline decltype(auto) as_int() { return std::any_cast<long long&>(data); }

    inline decltype(auto) as_float() { return std::any_cast<double&>(data); }

    inline decltype(auto) as_string() { return std::any_cast<std::string&>(data); }

    inline decltype(auto) as_bool() { return std::any_cast<bool&>(data); }

    inline decltype(auto) as_char() { return std::any_cast<char&>(data); }

    inline decltype(auto) as_list() { return std::any_cast<std::vector<PropertyValue>&>(data); }

    inline decltype(auto) as_map() { return std::any_cast<std::map<std::string, PropertyValue>&>(data); }
};

/**
 * \brief Named parts of type, what can be converted and projected to string.
 */
using Properties = std::map<std::string, PropertyValue>;

Properties operator+(Properties&& lhs, Properties&& rhs) noexcept
{
    lhs.merge(rhs);
    return lhs;
}

void register_userdata_property_value(luwra::StateWrapper& state) noexcept
{
    state.registerUserType<dynser::PropertyValue>(
        // members
        { LUWRA_MEMBER(dynser::PropertyValue, as_int),
          LUWRA_MEMBER(dynser::PropertyValue, as_float),
          LUWRA_MEMBER(dynser::PropertyValue, as_string) },
        // meta-members
        {}
    );
}
}    // namespace dynser

LUWRA_DEF_REGISTRY_NAME(dynser::PropertyValue, "PropertyValue")
