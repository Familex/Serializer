#pragma once

#include "dynser.h"

namespace dynser::util
{

Properties map_to_props() noexcept { return {}; }

template <typename T>
Properties map_to_props(T&&) noexcept = delete;

template <typename Key, typename Val, typename... Other>
Properties map_to_props(Key&& key, Val&& val, Other&&... other) noexcept
{
    return Properties{ { std::forward<Key>(key), PropertyValue{ std::forward<Val>(val) } } }
           << map_to_props(std::forward<Other>(other)...);
}

}    // namespace dynser::util
