#include "to_string.h"

using namespace dynser::config;

// regex::to_string impl
namespace
{
template <typename... Fs>
struct Overload : Fs...
{
    using Fs::operator()...;
};

/**
 * \brief Apply minimal possible quantifier to string.
 */
constexpr std::string apply_quantifier(const std::string_view s, const details::regex::Quantifier& quantifier) noexcept
{
    std::string result;
    for (std::size_t c{}; c < quantifier.from; ++c) {
        result += s;
    }
    return result;
}

/**
 * \brief Handle cases where we can remove/add symbols to string to make it fit the regex.
 * e.g. try_relent("1", /\d+{2}/) -> "01"
 * e.g. try_relent("001", /\d{2}/) -> "01"
 */
std::optional<std::string> try_relent(const std::string_view sv, const details::regex::Regex& reg) noexcept
{
    using namespace details::regex;
    using Result = std::optional<std::string>;

    return [&]() -> Result {
        // for \d and \w
        if (reg.value.size() != 1) {
            // more than one token
            return std::nullopt;
        }
        const auto& first_sus = reg.value[0];
        if (!std::holds_alternative<CharacterClass>(first_sus)) {
            // not []
            return std::nullopt;
        }
        const auto& first = std::get<CharacterClass>(first_sus);
        if (first.is_negative || first.characters.size() != 2 || first.characters[0] != '\\') {
            // ^ or not \_
            return std::nullopt;
        }
        if (first.characters[1] == 'd') {
            // \d case
            const auto is_all_digits = std::all_of(sv.begin(), sv.end(), [](const char c) { return std::isdigit(c); });
            if (!is_all_digits) {
                return std::nullopt;
            }
            if (first.quantifier.from > sv.size()) {
                return std::string(first.quantifier.from - sv.size(), '0') + std::string{ sv };
            }
            if (first.quantifier.to && *first.quantifier.to < sv.size()) {
                const auto leading_zeros_len = sv.size() - *first.quantifier.to;
                const auto leading_zeros_sus = sv.substr(0, leading_zeros_len);
                const auto is_leading_zeros_zeros =
                    std::all_of(leading_zeros_sus.begin(), leading_zeros_sus.end(), [](const char c) {
                        return c == '0';
                    });
                if (!is_leading_zeros_zeros) {
                    // try_relent("00010010", /\d{1,2}/) case
                    return std::nullopt;
                }
                return std::string{ sv.substr(leading_zeros_len) };
            }
        }
        if (first.characters[1] == 'w') {
            // \w case
            // we only can add spaces before sv
            if (first.quantifier.from > sv.size()) {
                // add spaces
                return std::string(first.quantifier.from - sv.size(), ' ') + std::string{ sv };
            }
        }
        return std::nullopt;
    }();
}

using CachedGroupValues = std::unordered_map<std::size_t, std::string>;

details::regex::ToStringResult resolve_regex(
    const details::regex::Regex& reg,
    const ::dynser::config::details::yaml::GroupValues& vals,
    CachedGroupValues& cached_group_values
) noexcept;    // forward declaration

details::regex::ToStringResult resolve_token(
    const details::regex::Token& tok,
    const ::dynser::config::details::yaml::GroupValues& vals,
    CachedGroupValues& cached_group_values
) noexcept
{
    using namespace details::regex;

    return std::visit(
        Overload{
            [&](const Empty& value) -> ToStringResult { return ""; },
            [&](const WildCard& value) -> ToStringResult { return apply_quantifier(".", value.quantifier); },
            [&](const Group& value) -> ToStringResult {
                if (!value.is_capturing) {
                    const auto result_sus = resolve_regex(*value.value, vals, cached_group_values);
                    if (!result_sus) {
                        return std::unexpected{ result_sus.error() };
                    }
                    return apply_quantifier(*result_sus, value.quantifier);
                }
                if (!vals.contains(value.number)) {
                    return std::unexpected{ ToStringError{ ToStringErrorType::MissingValue, value.number } };
                }
                std::string str_group_val = vals.at(value.number);
                if (!std::regex_match(str_group_val, value.regex)) {
                    if (auto appropriate_group_val = try_relent(str_group_val, *value.value)) {
                        str_group_val = std::move(*appropriate_group_val);
                    }
                    else {
                        return std::unexpected{ ToStringError{ ToStringErrorType::InvalidValue,
                                                               value.number } };    // can't fix wrong group val
                    }
                }
                cached_group_values[value.number] = str_group_val;
                return apply_quantifier(str_group_val, value.quantifier);
            },
            [&](const Backreference& value) -> ToStringResult {
                // future groups can't be inserted (by regex rules, i guess)
                if (cached_group_values.contains(value.group_number)) {
                    return apply_quantifier(cached_group_values.at(value.group_number), value.quantifier);
                }
                return std::unexpected{ ToStringError{ ToStringErrorType::MissingValue, value.group_number } };
            },
            [&](const Lookup& value) -> ToStringResult {
                return resolve_regex(*value.value, vals, cached_group_values);
            },
            [&](const CharacterClass& value) -> ToStringResult {
                // Get most left character and make it actual value

                if (value.characters.empty()) {
                    return std::unexpected{ ToStringError{
                        ToStringErrorType::InvalidValue,
                        static_cast<std::size_t>(-1)    // FIXME Wrong way to handle errors
                    } };
                }
                // Negative character class (like [^a-z])
                if (value.is_negative) {
                    return "?";    // FIXME not implemented
                }
                // One unescaped symbol, no exceptions, just return it
                if (value.characters[0] != '\\') {
                    return apply_quantifier(value.characters.substr(0, 1), value.quantifier);
                }
                if (value.characters.size() == 1) {
                    return std::unexpected{ ToStringError{
                        ToStringErrorType::InvalidValue,
                        static_cast<std::size_t>(-1)    // FIXME Wrong way to handle errors
                    } };
                }
                // Escaped character handle
                const auto escaped_char = value.characters[1];
                auto result_char{ escaped_char };
                switch (escaped_char) {
                    case 'd':
                        result_char = '0';
                        break;
                    case 'D':
                        result_char = 'D';
                        break;
                    case 'w':
                        result_char = 'w';
                        break;
                    case 'W':
                        result_char = '%';
                        break;
                    case 's':
                        result_char = ' ';
                        break;
                    case 'S':
                        result_char = 'S';
                        break;
                    case 't':
                        result_char = '\t';
                        break;
                    case 'r':
                        result_char = '\r';
                        break;
                    case 'n':
                        result_char = '\n';
                        break;
                    case 'v':
                        result_char = '\v';
                        break;
                    case 'f':
                        result_char = '\f';
                        break;
                    case '0':
                        result_char = '\0';
                        break;
                    default:
                        // Some syntax character or wrong character escaped, pass through
                        break;
                }
                return apply_quantifier(std::string(1, result_char), value.quantifier);
            },
            [&](const Disjunction& value) -> ToStringResult {
                return resolve_token(*value.left, vals, cached_group_values);
            },
        },
        tok
    );
}

details::regex::ToStringResult resolve_regex(
    const details::regex::Regex& reg,
    const ::dynser::config::details::yaml::GroupValues& vals,
    CachedGroupValues& cached_group_values
) noexcept
{
    using namespace details::regex;

    std::string result;
    for (const auto& token : reg.value) {
        if (const auto str_sus = resolve_token(token, vals, cached_group_values)) {
            result += *str_sus;
        }
        else {
            return std::unexpected{ str_sus.error() };
        }
    }
    return result;
}
}    // namespace

details::regex::ToStringResult details::regex::to_string(const Regex& reg, const yaml::GroupValues& vals) noexcept
{
    CachedGroupValues cached_group_values;    // for backreferences
    return ::resolve_regex(reg, vals, cached_group_values);
}

