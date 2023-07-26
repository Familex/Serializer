#pragma once

#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <variant>

namespace dynser::regex
{

// inclusive both ends
struct Quantifier
{
    std::size_t from;
    std::optional<std::size_t> to;
    bool is_lazy;

    constexpr inline auto operator<=>(const Quantifier&) const noexcept = default;
} inline const without_quantifier{ 1, 1, false };

using Token = std::variant<
    struct Empty,
    struct WildCard,
    struct Group,
    struct NonCapturingGroup,
    struct Backreference,
    struct Lookup,
    struct CharacterClass,
    struct Disjunction>;

struct Empty
{ };

// fullstop symbol
struct WildCard
{
    const Quantifier quantifier;
};

struct Group
{
    const std::unique_ptr<struct Regex> value;
    const Quantifier quantifier;

    // to vals check in regex::to_string
    const std::regex regex;

    // generated in regex::from_string
    const std::size_t number;

    explicit Group(
        std::unique_ptr<struct Regex>&& value,
        Quantifier&& quantifier,
        std::regex&& regex,
        std::size_t number
    ) noexcept;
    explicit Group(Group&& other) noexcept = default;
    Group(const Group& other) noexcept;
};

struct NonCapturingGroup
{
    const std::unique_ptr<struct Regex> value;
    const Quantifier quantifier;

    const std::regex regex;

    explicit NonCapturingGroup(
        std::unique_ptr<struct Regex>&& value,
        Quantifier&& quantifier,
        std::regex&& regex
    ) noexcept;
    explicit NonCapturingGroup(NonCapturingGroup&& other) noexcept = default;
    NonCapturingGroup(const NonCapturingGroup& other) noexcept;
};

struct Backreference
{
    const std::size_t group_number;
    const Quantifier quantifier;
};

struct Lookup
{
    const std::unique_ptr<struct Regex> value;
    const bool is_negative;
    const bool is_forward;    // true if forward lookup, false if backward

    explicit Lookup(std::unique_ptr<struct Regex>&& value, bool is_negative, bool is_forward) noexcept;
    explicit Lookup(Lookup&& other) noexcept = default;
    Lookup(const Lookup& other) noexcept;
};

struct CharacterClass
{
    const std::string characters;    // raw (without negation '^' symbol)
    const bool is_negative;
    const Quantifier quantifier;
};

struct Disjunction
{
    const std::unique_ptr<Token> left;
    const std::unique_ptr<Token> right;

    explicit Disjunction(std::unique_ptr<Token>&& left, std::unique_ptr<Token>&& right) noexcept;
    explicit Disjunction(Disjunction&& other) noexcept = default;
    Disjunction(const Disjunction& other) noexcept;
};

struct Regex
{
    const std::vector<Token> value;
};

}    // namespace dynser::regex
