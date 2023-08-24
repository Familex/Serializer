#pragma once

#include <variant>

namespace dynser::util
{
template <typename... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};
template <typename... Ts>
Overload(Ts...) -> Overload<Ts...>;

template <typename InstantiationTime>
struct Terminator : std::false_type
{ };

template <typename T, typename... Fs>
constexpr auto visit_one_terminated(T&& value, Fs&&... overloads) noexcept
{
    return visit_one(
        //
        std::forward<T>(value),
        std::forward<Fs>(overloads)...,
        [](auto const& unhandled_type) {
            static_assert(Terminator<decltype(unhandled_type)>::value, "type not handled");
        }
        //
    );
}

template <typename T, typename... Fs>
constexpr auto visit_one(T&& value, Fs&&... overloads) noexcept
{
    return std::visit(Overload{ std::forward<Fs>(overloads)... }, std::forward<T>(value));
}
}    // namespace dynser::util
