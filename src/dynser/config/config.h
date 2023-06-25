#pragma once

#include <unordered_map>

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace dynser::config
{

struct FileName
{
    std::string config_file_name;
};

struct RawContents
{
    std::string config;
};

namespace details::yaml
{
struct Regex
{
    std::string value;
};

struct DynRegex
{
    std::string value;
};

using GroupValues = std::unordered_map<std::size_t, std::string>;

using DynGroupValues = std::unordered_map<std::size_t, std::string>;
}    // namespace details::yaml

namespace details
{
// regex parsing
namespace regex
{

// inclusive both ends
struct Quantifier
{
    std::size_t from;
    std::optional<std::size_t> to;
    bool is_lazy;
} inline const without_quantifier{ 1, 1, false };

struct Empty
{ };

// fullstop symbol
struct WildCard
{
    Quantifier quantifier;
};

struct Group
{
    std::unique_ptr<struct Regex> value;
    bool is_capturing;
    Quantifier quantifier;

    explicit Group(std::unique_ptr<struct Regex>&& value, bool is_capturing, Quantifier&& quantifier) noexcept;
    explicit Group(Group&& other) noexcept = default;
    Group(const Group& other) noexcept;
};

struct Backreference
{
    std::size_t group_number;
    Quantifier quantifier;
};

// maybe in other struct
struct Lookup
{
    std::unique_ptr<struct Regex> value;
    bool is_negative;

    explicit Lookup(std::unique_ptr<struct Regex>&& value, bool is_negative) noexcept;
    explicit Lookup(Lookup&& other) noexcept = default;
    Lookup(const Lookup& other) noexcept;
};

// how to construct
struct CharacterClass
{
    std::string characters;    // raw (without negation '^' symbol)
    bool is_negative;
    Quantifier quantifier;
};

// ok
struct Disjunction
{
    std::unique_ptr<struct Regex> left;
    std::unique_ptr<struct Regex> right;

    explicit Disjunction(std::unique_ptr<struct Regex>&& left, std::unique_ptr<struct Regex>&& right) noexcept;
    explicit Disjunction(Disjunction&& other) noexcept = default;
    Disjunction(const Disjunction& other) noexcept;
};

using Token = std::variant<Empty, WildCard, Group, Backreference, Lookup, CharacterClass, Disjunction>;

struct Regex
{
    std::vector<Token> value;
};

// error position if error
std::expected<Regex, std::size_t> from_string(const std::string_view sv) noexcept;

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

}    // namespace regex

yaml::Regex resolve_dyn_regex(yaml::DynRegex&& dyn_reg, yaml::DynGroupValues&& dyn_gr_vals) noexcept;

regex::ToStringResult resolve_regex(yaml::Regex&& reg, yaml::GroupValues&& vals) noexcept;

}    // namespace details

struct Config
{
    explicit Config() noexcept { }

    Config(std::string_view config) noexcept
    {
        // TODO some config parse etc.
    }
};

}    // namespace dynser::config
