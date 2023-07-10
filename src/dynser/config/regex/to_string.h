#pragma once

#include "../structures.h"
#include "structures.h"

#include <expected>
#include <variant>

namespace dynser::config::details::regex
{

namespace to_string_err
{

struct RegexSyntaxError
{
    std::size_t position{};
};

struct MissingValue
{ };

struct InvalidValue
{
    std::string value;
};

}    // namespace to_string_err

struct ToStringError
{
    std::variant<
        to_string_err::RegexSyntaxError,    //
        to_string_err::MissingValue,
        to_string_err::InvalidValue>
        error;
    std::size_t group_num{};
};

using ToStringResult = std::expected<std::string, ToStringError>;

ToStringResult to_string(const Regex& reg, const yaml::GroupValues& vals) noexcept;

}    // namespace dynser::config::details::regex
