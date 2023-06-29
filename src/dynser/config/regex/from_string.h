#pragma once

#include "structures.h"

#include <expected>

namespace dynser::config::details::regex
{

// error position if error
std::expected<Regex, std::size_t> from_string(const std::string_view sv) noexcept;

}    // namespace dynser::config::details::regex
