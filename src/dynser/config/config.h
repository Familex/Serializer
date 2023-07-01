#pragma once

#include "regex/from_string.h"
#include "regex/to_string.h"
#include "structures.h"

namespace dynser::config
{

namespace details
{

yaml::Regex resolve_dyn_regex(const yaml::DynRegex& dyn_reg, const yaml::DynGroupValues& dyn_gr_vals) noexcept;

regex::ToStringResult resolve_regex(const yaml::Regex& reg, const yaml::GroupValues& vals) noexcept;

}    // namespace details

/**
 * \brief parse string and return config struct.
 * \throw if string is not valid yaml (see scheme).
 */
std::optional<Config> from_string(const std::string_view sv) noexcept;

}    // namespace dynser::config
