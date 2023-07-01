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

    // const

    inline decltype(auto) as_const_i32() const { return std::any_cast<const std::int32_t&>(data); }

    inline decltype(auto) as_const_i64() const { return std::any_cast<const std::int64_t&>(data); }

    inline decltype(auto) as_const_u32() const { return std::any_cast<const std::uint32_t&>(data); }

    inline decltype(auto) as_const_u64() const { return std::any_cast<const std::uint64_t&>(data); }

    inline decltype(auto) as_const_float() const { return std::any_cast<const double&>(data); }

    inline decltype(auto) as_const_string() const { return std::any_cast<const std::string&>(data); }

    inline decltype(auto) as_const_bool() const { return std::any_cast<const bool&>(data); }

    inline decltype(auto) as_const_char() const { return std::any_cast<const char&>(data); }

    inline decltype(auto) as_const_list() const { return std::any_cast<const std::vector<PropertyValue>&>(data); }

    inline decltype(auto) as_const_map() const
    {
        return std::any_cast<const std::map<std::string, PropertyValue>&>(data);
    }

    // not-const

    inline decltype(auto) as_i32() { return std::any_cast<const std::int32_t&>(data); }

    inline decltype(auto) as_i64() { return std::any_cast<const std::int64_t&>(data); }

    inline decltype(auto) as_u32() { return std::any_cast<const std::uint32_t&>(data); }

    inline decltype(auto) as_u64() { return std::any_cast<const std::uint64_t&>(data); }

    inline decltype(auto) as_float() { return std::any_cast<const double&>(data); }

    inline decltype(auto) as_string() { return std::any_cast<const std::string&>(data); }

    inline decltype(auto) as_bool() { return std::any_cast<const bool&>(data); }

    inline decltype(auto) as_char() { return std::any_cast<const char&>(data); }

    inline decltype(auto) as_list() { return std::any_cast<const std::vector<PropertyValue>&>(data); }

    inline decltype(auto) as_map() { return std::any_cast<const std::map<std::string, PropertyValue>&>(data); }
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
        {
            //
            LUWRA_MEMBER(dynser::PropertyValue, as_i32),
            LUWRA_MEMBER(dynser::PropertyValue, as_i64),
            LUWRA_MEMBER(dynser::PropertyValue, as_u32),
            LUWRA_MEMBER(dynser::PropertyValue, as_u64),
            LUWRA_MEMBER(dynser::PropertyValue, as_float),
            LUWRA_MEMBER(dynser::PropertyValue, as_string),
            LUWRA_MEMBER(dynser::PropertyValue, as_bool),
            LUWRA_MEMBER(dynser::PropertyValue, as_char),
            LUWRA_MEMBER(dynser::PropertyValue, as_list),
            LUWRA_MEMBER(dynser::PropertyValue, as_map),
            //
        },
        // meta-members
        {}
    );
}
}    // namespace dynser

LUWRA_DEF_REGISTRY_NAME(dynser::PropertyValue, "PropertyValue")
