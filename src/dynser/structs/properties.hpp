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

    decltype(auto) as_int() { return std::any_cast<long long&>(data); }

    decltype(auto) as_float() { return std::any_cast<double&>(data); }

    decltype(auto) as_string() { return std::any_cast<std::string&>(data); }
};

/**
 * \brief Named parts of type, what can be converted and projected to string.
 */
using Properties = std::map<std::string, PropertyValue>;

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
