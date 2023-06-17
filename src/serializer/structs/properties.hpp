#pragma once

#include "luwra.hpp"

#include <any>
#include <map>
#include <string>

namespace dyn_ser
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
    state.registerUserType<dyn_ser::PropertyValue>(
        // members
        { LUWRA_MEMBER(dyn_ser::PropertyValue, as_int),
          LUWRA_MEMBER(dyn_ser::PropertyValue, as_float),
          LUWRA_MEMBER(dyn_ser::PropertyValue, as_string) },
        // meta-members
        {}
    );
}
}    // namespace dyn_ser

LUWRA_DEF_REGISTRY_NAME(dyn_ser::PropertyValue, "PropertyValue")
