#pragma once

#include <type_traits>

template <typename T>
constexpr T clone(const T& t) noexcept(std::is_nothrow_copy_constructible_v<T>)
{
    return t;
}
