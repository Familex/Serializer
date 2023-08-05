#pragma once

#include "structures.h"

#include <expected>

namespace dynser::regex
{

// error position
using ParseError = std::size_t;

using ParseResult = std::expected<Regex, ParseError>;

// error position if error
ParseResult from_string(const std::string_view sv) noexcept;

}    // namespace dynser::regex
