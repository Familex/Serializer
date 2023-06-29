#pragma once

#include "structures.h"
#include "../structures.h"

#include <expected>

namespace dynser::config::details::regex
{

enum class ToStringErrorType {
    RegexSyntaxError,    /// group regex syntax error
    MissingValue,        /// value with current group num not found
    InvalidValue,        /// group regex missmatched
};

struct ToStringError
{
    ToStringErrorType type;
    std::size_t group_num;    // wrong field arrangement
};

using ToStringResult = std::expected<std::string, ToStringError>;

ToStringResult to_string(const Regex& reg, const yaml::GroupValues& vals) noexcept;

}    // namespace dynser::config::details::regex
