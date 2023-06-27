#include "config.h"

#include <charconv>
#include <expected>
#include <locale>
#include <optional>
#include <regex>

using namespace dynser::config;

namespace dynser::config::details::regex
{
Group::Group(
    std::unique_ptr<Regex>&& value,
    bool is_capturing,
    Quantifier&& quantifier,
    std::regex&& regex,
#ifdef _DEBUG
    std::string&& regex_str,
#endif
    std::size_t number
) noexcept
  : value{ std::move(value) }
  , is_capturing{ is_capturing }
  , quantifier{ std::move(quantifier) }
  , regex{ std::move(regex) }
#ifdef _DEBUG
  , regex_str{ std::move(regex_str) }
#endif
  , number{ number }
{ }

Group::Group(const Group& other) noexcept
  : value{ new Regex{ *other.value } }
  , is_capturing{ other.is_capturing }
  , quantifier{ other.quantifier }
  , regex{ other.regex }
#ifdef _DEBUG
  , regex_str{ other.regex_str }
#endif
  , number{ other.number }
{ }

Lookup::Lookup(std::unique_ptr<Regex>&& value, bool is_negative, bool is_forward) noexcept
  : value{ std::move(value) }
  , is_negative{ is_negative }
  , is_forward{ is_forward }
{ }

Lookup::Lookup(const Lookup& other) noexcept
  : value{ new Regex{ *other.value } }
  , is_negative{ other.is_negative }
  , is_forward{ other.is_forward }
{ }

Disjunction::Disjunction(std::unique_ptr<Token>&& left, std::unique_ptr<Token>&& right) noexcept
  : left{ std::move(left) }
  , right{ std::move(right) }
{ }

Disjunction::Disjunction(const Disjunction& other) noexcept
  : left{ new Token{ *other.left } }
  , right{ new Token{ *other.right } }
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

// regex::from_string impl
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

/**
 * \brief Return index of paired bracket of sv[resp_pos] else std::nullopt.
 * \tparam open_bracket open bracket char.
 * \tparam close_bracket close bracket char.
 */
template <char open_bracket, char close_bracket>
constexpr std::optional<std::size_t> find_paired_bracket(const std::string_view sv, const std::size_t resp_pos) noexcept
{
    if (sv[resp_pos] != open_bracket) {
        return std::nullopt;
    }
    std::size_t open_brackets = 1;
    for (std::size_t i = resp_pos + 1; i < sv.size(); ++i) {
        if (sv[i] == open_bracket) {
            ++open_brackets;
        }
        else if (sv[i] == close_bracket) {
            --open_brackets;
            if (open_brackets == 0) {
                return i;
            }
        }
    }
    return std::nullopt;
}

std::expected<details::regex::Regex, std::size_t>
parse_regex(const std::string_view sv, std::size_t& last_group_number) noexcept;    // forward declaration

/**
 * \brief Return parsed token (from sv begin) and it length or parse error position.
 */
std::expected<std::pair<details::regex::Token, std::size_t>, std::size_t> parse_token(
    const std::string_view sv,
    std::vector<details::regex::Token>& result,
    std::size_t& last_group_number
) noexcept
{
    using namespace details::regex;

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

            return {
                { CharacterClass{
                      std::string{ sv.substr(character_class_begin, character_class_end - character_class_begin + 1) },
                      is_negated,
                      search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier) },
                  token_len }
            };
        }
        // disjunction
        case '|':
        {
            auto right_sus = parse_token(sv.substr(1), result, last_group_number);
            if (!right_sus) {
                return std::unexpected{ right_sus.error() };
            }
            auto&& [right, right_len] = std::move(*right_sus);
            if (result.empty()) {
                return {
                    { { Disjunction{ std::make_unique<Token>(Empty{}), std::make_unique<Token>(std::move(right)) } },
                      right_len + 1 }
                };
            }
            else {
                Token left = std::move(result.back());
                result.pop_back();
                return { { Disjunction{ std::make_unique<Token>(std::move(left)),
                                        std::make_unique<Token>(std::move(right)) },

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
        // group | lookup
        case '(':
        {
            ++token_len;
            const auto is_lookahead{ sv.size() > 2 && sv[token_len] == '?' &&
                                     (sv[token_len + 1] == '!' || sv[token_len + 1] == '=') };
            const auto is_lookbehind{ sv.size() > 3 && sv[token_len] == '?' && sv[token_len + 1] == '<' &&
                                      (sv[token_len + 2] == '!' || sv[token_len + 2] == '=') };
            if (is_lookahead || is_lookbehind) {
                const auto is_forward{ is_lookahead };
                const auto is_negative{ is_lookahead ? sv[token_len + 1] == '!' : sv[token_len + 2] == '!' };
                token_len += is_lookahead ? 2 : 3;

                // group parse
                const auto group_end_sus = find_paired_bracket<'(', ')'>(sv, 0);
                if (!group_end_sus) {
                    return std::unexpected{ token_len };    // missmatched open bracket
                }
                const auto group_len = *group_end_sus - token_len;
                auto group_sus = parse_regex(sv.substr(token_len, group_len), last_group_number);
                if (!group_sus) {
                    return std::unexpected{ group_sus.error() };
                }
                auto&& group = std::move(*group_sus);
                token_len += group_len + 1;    // skip ')'

                return { { Lookup{ std::make_unique<Regex>(std::move(group)), is_negative, is_forward }, token_len } };
            }
            const bool is_not_capturing{ sv.size() > 2 && sv[token_len] == '?' && sv[token_len + 1] == ':' };
            if (is_not_capturing) {
                token_len += 2;
            }
            // group parse
            const auto group_start = token_len;
            const auto group_end_sus = find_paired_bracket<'(', ')'>(sv, 0);
            if (!group_end_sus) {
                return std::unexpected{ token_len };    // missmatched open bracket
            }
            const auto group_len = *group_end_sus - token_len;
            const auto current_group_number =
                ++last_group_number;    // remember own group number, 'cause numbering in BFS
            std::string group_str{ sv.substr(group_start, group_len) };
            auto group_sus = parse_regex(group_str, last_group_number);
            if (!group_sus) {
                return std::unexpected{ group_sus.error() };
            }
            std::regex regex;
            try {
                regex = std::regex{ group_str.data(), group_str.size() };
            }
            catch (const std::regex_error&) {
                return std::unexpected{ group_start };
            }
            auto&& group = std::move(*group_sus);
            token_len += group_len + 1;    // skip ')'
            return { { Group{ std::make_unique<Regex>(std::move(group)),
                              !is_not_capturing,
                              search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier),
                              std::move(regex),
#ifdef _DEBUG
                              std::move(group_str),
#endif
                              current_group_number },
                       token_len } };
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
            characters += sv[token_len - 1];    // FIXME make it in place (std::string{ 1, `char` }) doesn't worked
            return { { CharacterClass{
                           characters,
                           false,
                           search_quantifier(sv.substr(token_len), &token_len).value_or(without_quantifier) },
                       token_len } };
    }
}

/**
 * \brief Parse regex string into regex::Regex.
 * \param sv string to parse.
 * \param [in,out] last_group_number last group number in parse process.
 * \return parsed regex.
 */
std::expected<details::regex::Regex, std::size_t>
parse_regex(const std::string_view sv, std::size_t& last_group_number) noexcept
{
    using namespace details::regex;

    std::vector<Token> result;
    std::size_t curr{};
    while (curr < sv.size()) {
        auto res_sus = parse_token(sv.substr(curr), result, last_group_number);
        if (!res_sus) {
            return std::unexpected{ curr + res_sus.error() };
        }
        auto&& [token, len] = std::move(*res_sus);
        result.push_back(std::move(token));
        curr += len;
    }
    return Regex{ result };
}
}    // namespace

std::expected<details::regex::Regex, std::size_t> details::regex::from_string(const std::string_view sv) noexcept
{
    // lvalue-reference to [in,out] param
    std::size_t last_group_number{};
    return parse_regex(sv, last_group_number);
}

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
                // is_capturing is unused?
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
                    return apply_quantifier(value.characters, value.quantifier);
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
