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

    constexpr inline auto operator<=>(Quantifier const&) const noexcept = default;
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
    Quantifier quantifier;
};

struct Group
{
    std::unique_ptr<struct Regex const> value;
    Quantifier quantifier;

    // to vals check in regex::to_string
    std::regex regex;

    // generated in regex::from_string
    std::size_t number;

    explicit Group(
        std::unique_ptr<struct Regex const>&& value,
        Quantifier&& quantifier,
        std::regex&& regex,
        std::size_t number
    ) noexcept;
    Group(Group&&) noexcept = default;
    Group(Group const& other) noexcept;
};

struct NonCapturingGroup
{
    std::unique_ptr<struct Regex const> value;
    Quantifier quantifier;

    std::regex regex;

    explicit NonCapturingGroup(
        std::unique_ptr<struct Regex const>&& value,
        Quantifier&& quantifier,
        std::regex&& regex
    ) noexcept;
    NonCapturingGroup(NonCapturingGroup&& other) noexcept = default;
    NonCapturingGroup(NonCapturingGroup const& other) noexcept;
};

struct Backreference
{
    std::size_t group_number;
    Quantifier quantifier;
};

struct Lookup
{
    std::unique_ptr<struct Regex const> value;
    bool is_negative;
    bool is_forward;    // true if forward lookup, false if backward

    explicit Lookup(std::unique_ptr<struct Regex const>&& value, bool is_negative, bool is_forward) noexcept;
    Lookup(Lookup&&) noexcept = default;
    Lookup(Lookup const& other) noexcept;
};

struct CharacterClass
{
    std::string characters;    // raw (without negation '^' symbol)
    bool is_negative;
    Quantifier quantifier;
};

struct Disjunction
{
    std::unique_ptr<Token const> left;
    std::unique_ptr<Token const> right;

    explicit Disjunction(std::unique_ptr<Token const>&& left, std::unique_ptr<Token const>&& right) noexcept;
    Disjunction(Disjunction&&) noexcept = default;
    Disjunction(Disjunction const& other) noexcept;
};

struct Regex
{
    std::vector<Token> value;
};

}    // namespace dynser::regex
