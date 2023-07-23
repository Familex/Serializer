#include "from_string.h"

#include <charconv>

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
std::optional<dynser::regex::Quantifier>
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
                return dynser::regex::Quantifier{ *count_sus, *count_sus, is_lazy };
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
                return dynser::regex::Quantifier{ *from_sus, has_to ? *to_sus : to_sus, is_lazy };
            }
        } break;
        case '+':
        {
            const auto is_lazy = lazy_check(sv, 1);
            if (out) {
                *out += static_cast<std::size_t>(1) + is_lazy;
            }
            return dynser::regex::Quantifier{ 1, std::nullopt, is_lazy };
        } break;
        case '*':
        {
            const auto is_lazy = lazy_check(sv, 1);
            if (out) {
                *out += static_cast<std::size_t>(1) + is_lazy;
            }
            return dynser::regex::Quantifier{ 0, std::nullopt, is_lazy };
        } break;
        case '?':
        {
            const auto is_lazy = lazy_check(sv, 1);
            if (out) {
                *out += static_cast<std::size_t>(1) + is_lazy;
            }
            return dynser::regex::Quantifier{ 0, 1, is_lazy };
        } break;
    }
    return std::nullopt;
}

/**
 * \brief Return index of paired bracket of sv[resp_pos] else std::nullopt.
 */
constexpr std::optional<std::size_t>
find_paired_round_bracket(const std::string_view sv, const std::size_t resp_pos) noexcept
{
    constexpr auto open_bracket = '(';
    constexpr auto close_bracket = ')';
    constexpr auto open_character_class = '[';
    constexpr auto close_character_class = ']';

    if (sv[resp_pos] != open_bracket) {
        return std::nullopt;
    }

    // ([)](\w)){2}

    bool is_character_class{ false };
    int is_escaped{ 0 };    // works like timer
    std::size_t open_brackets = 1;
    for (std::size_t i = resp_pos + 1; i < sv.size(); ++i) {
        if (is_escaped > 0) {
            --is_escaped;
        }
        if (sv[i] == '\\') {
            is_escaped = 2;
        }
        else if (!is_character_class && !is_escaped) {
            // groups
            switch (sv[i]) {
                case open_bracket:
                    ++open_brackets;
                    break;
                case close_bracket:
                    --open_brackets;
                    break;
                case open_character_class:
                    is_character_class = true;
                    break;
            }
            if (open_brackets == 0) {
                return i;
            }
        }
        else if (sv[i] == close_character_class) {
            is_character_class = false;
        }
    }
    return std::nullopt;
}

std::expected<dynser::regex::Regex, std::size_t>
parse_regex(const std::string_view sv, std::size_t& last_group_number) noexcept;    // forward declaration

/**
 * \brief Return parsed token (from sv begin) and it length or parse error position.
 */
std::expected<std::pair<dynser::regex::Token, std::size_t>, std::size_t> parse_token(
    const std::string_view sv,
    std::vector<dynser::regex::Token>& result,
    std::size_t& last_group_number
) noexcept
{
    using namespace dynser::regex;

    std::size_t token_len{};

    if (sv.empty()) {
        return std::unexpected{ token_len };
    }

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
            if (sv.size() <= token_len) {
                // ']' can't be here, what is syntax error
                return std::unexpected{ token_len };
            }
            const auto is_negated = sv[token_len] == '^';
            // search closing bracket (be aware of escaped variant)
            std::size_t character_class_begin{ token_len + is_negated };
            std::size_t character_class_end{ token_len + 1 };
            while (character_class_end < sv.size() &&
                   !(sv[character_class_end] == ']' && sv[character_class_end - 1] != '\\'))
            {
                ++character_class_end;
            }
            if (character_class_end == sv.size()) {
                // ']' not found
                return std::unexpected{ character_class_end };
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
                const auto group_end_sus = find_paired_round_bracket(sv, 0);
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
            const auto group_end_sus = find_paired_round_bracket(sv, 0);
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
std::expected<dynser::regex::Regex, std::size_t>
parse_regex(const std::string_view sv, std::size_t& last_group_number) noexcept
{
    using namespace dynser::regex;

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

std::expected<dynser::regex::Regex, std::size_t> dynser::regex::from_string(const std::string_view sv) noexcept
{
    // lvalue-reference to [in,out] param
    std::size_t last_group_number{};
    return parse_regex(sv, last_group_number);
}
