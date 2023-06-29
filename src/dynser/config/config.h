#pragma once

#include "regex/from_string.h"
#include "regex/to_string.h"
#include "structures.h"

namespace dynser::config
{

namespace details
{

yaml::Regex resolve_dyn_regex(yaml::DynRegex&& dyn_reg, yaml::DynGroupValues&& dyn_gr_vals) noexcept;

regex::ToStringResult resolve_regex(yaml::Regex&& reg, yaml::GroupValues&& vals) noexcept;

}    // namespace details

struct Config
{
    explicit Config() noexcept { }

    Config(std::string_view config) noexcept
    {
        // TODO some config parse etc.
    }
};

}    // namespace dynser::config
