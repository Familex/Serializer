#include "config.h"

#include <charconv>
#include <expected>
#include <locale>
#include <optional>
#include <regex>

using namespace dynser::config;

namespace dynser::config::details::regex
{
Group::Group(std::unique_ptr<Regex>&& value, bool is_capturing, Quantifier&& quantifier) noexcept
  : value{ std::move(value) }
  , is_capturing{ is_capturing }
  , quantifier{ std::move(quantifier) }
{ }

Group::Group(const Group& other) noexcept
  : value{ new Regex{ *other.value } }
  , is_capturing{ other.is_capturing }
  , quantifier{ other.quantifier }
{ }

Lookup::Lookup(std::unique_ptr<Regex>&& value, bool is_negative) noexcept
  : value{ std::move(value) }
  , is_negative{ is_negative }
{ }

Lookup::Lookup(const Lookup& other) noexcept
  : value{ new Regex{ *other.value } }
  , is_negative{ other.is_negative }
{ }

Disjunction::Disjunction(std::unique_ptr<Regex>&& left, std::unique_ptr<Regex>&& right) noexcept
  : left{ std::move(left) }
  , right{ std::move(right) }
{ }

Disjunction::Disjunction(const Disjunction& other) noexcept
  : left{ new Regex{ *other.left } }
  , right{ new Regex{ *other.right } }
{ }
}    // namespace dynser::config::details::regex

// https://stackoverflow.com/a/37516316
namespace
{
template <class BidirIt, class Traits, class CharT, class UnaryFunction>
std::basic_string<CharT>
regex_replace(BidirIt first, BidirIt last, const std::basic_regex<CharT, Traits>& re, UnaryFunction f)
{
    std::basic_string<CharT> s;

    typename std::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
    auto endOfLastMatch = first;

    auto callback = [&](const std::match_results<BidirIt>& match) {
        auto positionOfThisMatch = match.position(0);
        auto diff = positionOfThisMatch - positionOfLastMatch;

        auto startOfThisMatch = endOfLastMatch;
        std::advance(startOfThisMatch, diff);

        s.append(endOfLastMatch, startOfThisMatch);
        s.append(f(match));

        auto lengthOfMatch = match.length(0);

        positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

        endOfLastMatch = startOfThisMatch;
        std::advance(endOfLastMatch, lengthOfMatch);
    };

    std::regex_iterator<BidirIt> begin(first, last, re), end;
    std::for_each(begin, end, callback);

    s.append(endOfLastMatch, last);

    return s;
}

template <class Traits, class CharT, class UnaryFunction>
std::string regex_replace(const std::string& s, const std::basic_regex<CharT, Traits>& re, UnaryFunction f)
{
    return regex_replace(s.cbegin(), s.cend(), re, f);
}
}    // namespace

details::yaml::Regex details::resolve_dyn_regex(yaml::DynRegex&& dyn_reg, yaml::DynGroupValues&& dyn_gr_vals) noexcept
{
    static const std::regex dyn_gr_pattern{ R"(\\_(\d+))" };
    return { regex_replace(dyn_reg.value, dyn_gr_pattern, [&dyn_gr_vals](const std::smatch& m) {
        const auto gr_num = std::atoi(m.str(1).c_str());
        if (dyn_gr_vals.contains(gr_num)) {
            return dyn_gr_vals.at(gr_num);
        }
        return std::string{};
    }) };
}

namespace
{
std::optional<std::string> try_relent(const std::string_view sv, const details::regex::Regex& reg) noexcept
{
    // FIXME implement
    // e.g. try_relent("14", "\d{4}") → "0014"
    return std::string{ sv };
}
}    // namespace

details::regex::ToStringResult details::resolve_regex(yaml::Regex&& reg, yaml::GroupValues&& vals) noexcept
{
    using namespace details::regex;

    const auto reg_sus = from_string(reg.value);
    if (!reg_sus) {
        return std::unexpected{ ToStringError{
            ToStringErrorType::RegexSyntaxError,
            0    // group number
        } };
    }
    return to_string(*reg_sus, std::move(vals));
}

namespace
{
/**
 * \brief Converts a string_view to an int if all string is convertible.
 */
constexpr auto inline svtoi(const std::string_view s) noexcept -> std::optional<int>
{
    int value{};
    const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec == std::errc{}) {
        return value;
    }
    return std::nullopt;
}

template <typename T>
auto inline number_len(T number) noexcept
{
    return static_cast<std::size_t>(trunc(log10(number))) + 1ull;
}

/**
 * \brief Return Quantifier if exists at sv start and add it length to out if provided.
 */
std::optional<details::regex::Quantifier>
search_quantifier(const std::string_view sv, std::size_t* out /* = nullptr */) noexcept
{
    static const auto lazy_check = [](const std::string_view sv, const std::size_t offset) {
        return sv.size() > offset && sv[offset] == '?';
    };

    if (sv.empty())
        return std::nullopt;

    switch (sv[0]) {
        case '{':
        {
            const auto quantifier_end = sv.find('}');
            if (quantifier_end == sv.npos) {
                // FIXME syntax error miss
                break;
            }
            const auto comma_pos = sv.find(',');
            if (comma_pos == sv.npos) {
                const auto count_str = sv.substr(1, quantifier_end - 1);
                const std::optional<std::size_t> count_sus = svtoi(count_str);
                if (!count_sus) {
                    // FIXME syntax error miss
                    break;
                }
                const auto is_lazy = lazy_check(sv, quantifier_end + 1);
                if (out) {
                    *out += quantifier_end + is_lazy + 1;
                }
                return details::regex::Quantifier{ *count_sus, *count_sus, is_lazy };
            }
            else {
                const auto from_str = sv.substr(1, comma_pos - 1);
                const auto to_str = sv.substr(comma_pos + 1, quantifier_end - comma_pos - 1);
                const std::optional<std::size_t> from_sus = svtoi(from_str);
                const auto has_to = !to_str.empty();
                // value or parse error or infinite quantifier
                const std::optional<std::size_t> to_sus = has_to ? svtoi(to_str) : std::nullopt;
                if (!from_sus || has_to && !to_sus) {
                    // FIXME syntax error miss
                    break;
                }
                const auto is_lazy = lazy_check(sv, quantifier_end + 1);
                if (out) {
                    *out += quantifier_end + is_lazy + 1;
                }
                return details::regex::Quantifier{ *from_sus, has_to ? *to_sus : to_sus, is_lazy };
            }
        } break;
        case '+':
        {
            const auto is_lazy = lazy_check(sv, 1);
            if (out) {
                *out += static_cast<std::size_t>(1) + is_lazy;
            }
            return details::regex::Quantifier{ 1, std::nullopt, is_lazy };
        } break;
        case '*':
        {
            const auto is_lazy = lazy_check(sv, 1);
            if (out) {
                *out += static_cast<std::size_t>(1) + is_lazy;
            }
            return details::regex::Quantifier{ 0, std::nullopt, is_lazy };
        } break;
        case '?':
        {
            const auto is_lazy = lazy_check(sv, 1);
            if (out) {
                *out += static_cast<std::size_t>(1) + is_lazy;
            }
            return details::regex::Quantifier{ 0, 1, is_lazy };
        } break;
    }
    return std::nullopt;
}
}    // namespace

std::expected<details::regex::Regex, std::size_t> details::regex::from_string(const std::string_view sv) noexcept
{
    Regex result;

    /**
     * \brief Return parsed token (from sv begin) and it length or parse error position.
     */
    static auto parse_token = [](this auto& self, const std::string_view sv, Regex& result
                              ) -> std::expected<std::pair<Token, std::size_t>, std::size_t> {
        if (sv.empty()) {
            return std::unexpected{ 0 };
        }

        std::size_t token_len{};
        switch (sv[0]) {
            // fullstop symbol
            case '.':
                ++token_len;
                return { { WildCard{ search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier) },
                           token_len } };
            // escaped character
            case '\\':
                ++token_len;
                if (std::isdigit(sv[token_len])) {
                    const auto backref = static_cast<std::size_t>(*svtoi(sv.substr(token_len)));
                    token_len += number_len(backref);
                    return { { Backreference{
                                   backref,
                                   search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier) },
                               token_len } };
                }
                else {
                    ++token_len;
                    return { { CharacterClass{
                                   std::string{ { '\\', sv[token_len - 1] } },
                                   false,
                                   search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier) },
                               token_len } };
                }
            // character class
            case '[':
            {
                ++token_len;
                const auto is_negated = sv[token_len] == '^';
                // search closing bracket (be aware of escaped variant)
                std::size_t character_class_begin{ token_len + is_negated };
                std::size_t character_class_end{ token_len + 1 };
                // FIXME sv end handle (sv without ']')
                while (sv[character_class_end] != ']' || sv[character_class_end - 1] == '\\') {
                    ++character_class_end;
                }
                --character_class_end;                  // skip ']'
                token_len = character_class_end + 2;    // skip ']'

                return { { CharacterClass{
                               std::string{
                                   sv.substr(character_class_begin, character_class_end - character_class_begin + 1) },
                               is_negated,
                               search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier) },
                           token_len } };
            }
            // disjunction
            case '|':
            {
                auto right_sus = self(sv.substr(1), result);
                if (!right_sus) {
                    return std::unexpected{ right_sus.error() };
                }
                auto&& [right, right_len] = std::move(*right_sus);
                if (result.value.empty()) {
                    return { { { Disjunction{ std::make_unique<Regex>(std::vector<Token>{ Empty{} }),
                                              std::make_unique<Regex>(std::vector<Token>{ std::move(right) }) } },
                               right_len + 1 } };
                }
                else {
                    Token left = result.value.back();
                    result.value.pop_back();
                    return { { Disjunction{ std::make_unique<Regex>(std::vector<Token>{ left }),
                                            std::make_unique<Regex>(std::vector<Token>{ std::move(right) }) },
                               right_len + 1 } };
                }
            }
            // illegal here (regex is context sensitive)
            case ']':
            case ')':
            case '{':
            case '}':
            case '+':
            case '*':
            case '?':
                return std::unexpected{ 0 };
            // group
            case '(':
            {
                ++token_len;
                // find matching bracket and make loop like in outher function
                bool is_capturing{ (sv.size() > 2 && sv[token_len] == '?' && sv[token_len + 1] == ':') };
                if (is_capturing) {
                    token_len += 2;
                }
                auto content_sus = self(sv.substr(token_len), result);
                if (!content_sus) {
                    return std::unexpected{ content_sus.error() };
                }
                auto&& [content, content_len] = std::move(*content_sus);
                token_len += content_len;
                return { { Group{ std::make_unique<Regex>(std::vector<Token>{ content }),
                                  is_capturing,
                                  search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier) },
                           token_len + 1 } };
            }
            // string begin (how to handle)
            case '^':
                // FIXME
                // fall to prevent warnings
                [[fallthrough]];
            // string end (how to handle)
            case '$':
                // FIXME
                [[fallthrough]];
            // just character (like escaped)
            default:
                ++token_len;
                std::string characters;
                characters += sv[token_len - 1];    // FIXME make it in place (std::string{ 1, bla-bla }) doesn't worked
                return { { CharacterClass{
                               characters,
                               false,
                               search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier) },
                           token_len } };
        }
    };

    std::size_t curr{};
    while (curr < sv.size()) {
        auto res_sus = parse_token(sv.substr(curr), result);
        if (!res_sus) {
            return std::unexpected{ curr + res_sus.error() };
        }
        auto&& [token, len] = std::move(*res_sus);
        result.value.push_back(std::move(token));
        curr += len;
    }
    return result;
}

details::regex::ToStringResult details::regex::to_string(const Regex& reg, const yaml::GroupValues& vals) noexcept
{
    // FIXME
    return std::string{};
}
