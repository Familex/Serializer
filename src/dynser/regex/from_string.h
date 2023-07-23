#pragma once

#include "structures.h"

#include <expected>

namespace dynser::regex
{

// error position if error
std::expected<Regex, std::size_t> from_string(const std::string_view sv) noexcept;

}    // namespace dynser::regex
