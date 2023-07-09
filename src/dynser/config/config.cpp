#include "config.h"

#include "keywords.h"
#include "yaml-cpp/yaml.h"

#include <charconv>
#include <expected>
#include <locale>
#include <optional>
#include <regex>

// debug print
#ifdef _DEBUG
#include <format>
#include <iostream>
#endif

using namespace dynser::config;

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

details::yaml::Regex
details::resolve_dyn_regex(const yaml::DynRegex& dyn_reg, const yaml::DynGroupValues& dyn_gr_vals) noexcept
{
    static const std::regex dyn_gr_pattern{ R"(\\_(\d+))" };
    return { regex_replace(dyn_reg, dyn_gr_pattern, [&dyn_gr_vals](const std::smatch& m) {
        const auto gr_num = std::atoi(m.str(1).c_str());
        if (dyn_gr_vals.contains(gr_num)) {
            return dyn_gr_vals.at(gr_num);
        }
        return std::string{};
    }) };
}

details::regex::ToStringResult details::resolve_regex(const yaml::Regex& reg, const yaml::GroupValues& vals) noexcept
{
    using namespace details::regex;

    auto reg_sus = from_string(reg);
    if (!reg_sus) {
        return std::unexpected{ ToStringError{
            ToStringErrorType::RegexSyntaxError,
            0    // group number
        } };
    }
    const auto regex = !vals.contains(0)
                           ? *reg_sus
                           : Regex{ std::vector<Token>{ Group{ std::make_unique<Regex>(std::move(*reg_sus)),
                                                               true,
                                                               Quantifier{ 1, 1, false },
                                                               std::regex{ reg.data(), reg.size() },
#ifdef _DEBUG
                                                               static_cast<std::string>(reg),
#endif
                                                               0 } } };
    return to_string(regex, vals);
}

// config::from_string helpers
namespace
{

template <typename Res>
inline std::optional<Res> as_opt(YAML::Node const& node) noexcept
{
    if (node.IsDefined()) {
        return node.as<Res>();
    }
    return std::nullopt;
};

}    // namespace

std::optional<Config> dynser::config::from_string(const std::string_view sv) noexcept
{
    // FIXME add tags to field names to prevent collisions

    try {
        using namespace dynser::config;
        using namespace dynser::config::details::yaml;

        const auto yaml = YAML::Load(std::string{ sv });
        Config result;

        result.version = yaml[keywords::VERSION].as<std::string>();

        for (const auto tag : yaml[keywords::TAGS]) {
            const auto tag_name = tag[keywords::NAME].as<std::string>();

            result.tags[tag_name] = {
                .name = tag_name,
                .nested = [&]() -> details::yaml::Nested {    // iife
                    if (const auto continual = tag[keywords::NESTED_CONTINUAL]) {
                        details::yaml::Continual nested;

                        for (const auto continual_rule_type : continual) {
                            if (const auto rule = continual_rule_type[keywords::CONTINUAL_EXISTING]) {
                                nested.push_back(details::yaml::ConExisting{
                                    .tag = rule[keywords::CONTINUAL_EXISTING_TAG].as<std::string>(),
                                    .prefix = as_opt<std::string>(rule[keywords::CONTINUAL_EXISTING_PREFIX]) });
                            }
                            else if (const auto rule = continual_rule_type[keywords::CONTINUAL_LINEAR]) {
                                nested.push_back(details::yaml::ConLinear{
                                    .pattern = rule[keywords::CONTINUAL_LINEAR_PATTERN].as<std::string>(),
                                    .dyn_groups = as_opt<details::yaml::DynGroupValues>(
                                        rule[keywords::CONTINUAL_LINEAR_DYN_GROUPS]
                                    ),
                                    .fields =
                                        as_opt<details::yaml::GroupValues>(rule[keywords::CONTINUAL_LINEAR_FIELDS]) });
                            }
                        }

                        return nested;
                    }
                    else if (const auto branched = tag[keywords::NESTED_BRANCHED]) {
                        details::yaml::Branched nested;

                        nested.branching_script = branched[keywords::BRANCHED_BRANCHING_SCRIPT].as<std::string>();
                        nested.debranching_script = branched[keywords::BRANCHED_DEBRANCHING_SCRIPT].as<std::string>();

                        for (const auto branched_rule_type : branched[keywords::BRANCHED_RULES]) {
                            if (const auto rule = branched_rule_type[keywords::BRANCHED_EXISTING]) {
                                nested.rules.push_back(details::yaml::BraExisting{
                                    .tag = rule[keywords::BRANCHED_EXISTING_TAG].as<std::string>(),
                                    .prefix = as_opt<std::string>(rule[keywords::BRANCHED_EXISTING_PREFIX]) });
                            }
                            else if (const auto rule = branched_rule_type[keywords::BRANCHED_LINEAR]) {
                                nested.rules.push_back(details::yaml::BraLinear{
                                    .pattern = rule[keywords::BRANCHED_LINEAR_PATTERN].as<std::string>(),
                                    .dyn_groups =
                                        as_opt<details::yaml::DynGroupValues>(rule[keywords::BRANCHED_LINEAR_DYN_GROUPS]
                                        ),
                                    .fields =
                                        as_opt<details::yaml::GroupValues>(rule[keywords::BRANCHED_LINEAR_FIELDS]) });
                            }
                        }

                        return nested;
                    }
                    else if (const auto recurrent = tag[keywords::NESTED_RECURRENT]) {
                        details::yaml::Recurrent nested;

                        for (const auto recurrent_rule_type : recurrent) {
                            if (const auto rule = recurrent_rule_type[keywords::RECURRENT_EXISTING]) {
                                nested.push_back(RecExisting{
                                    .tag = rule[keywords::RECURRENT_EXISTING_TAG].as<std::string>(),
                                    .prefix = rule[keywords::RECURRENT_EXISTING_PREFIX].as<std::string>(),
                                    .wrap = rule[keywords::RECURRENT_EXISTING_WRAP].as<bool>(),
                                    .default_value =
                                        as_opt<std::string>(rule[keywords::RECURRENT_EXISTING_DEFAULT_VALUE]),
                                });
                            }
                            else if (const auto rule = recurrent_rule_type[keywords::RECURRENT_LINEAR]) {
                                nested.push_back(RecLinear{
                                    .pattern = rule[keywords::RECURRENT_LINEAR_PATTERN].as<std::string>(),
                                    .dyn_groups = as_opt<details::yaml::DynGroupValues>(
                                        rule[keywords::RECURRENT_LINEAR_DYN_GROUPS]
                                    ),
                                    .fields =
                                        as_opt<details::yaml::GroupValues>(rule[keywords::RECURRENT_LINEAR_FIELDS]),
                                    .wrap = rule[keywords::RECURRENT_LINEAR_WRAP].as<bool>(),
                                    .default_value =
                                        as_opt<std::string>(rule[keywords::RECURRENT_LINEAR_DEFAULT_VALUE]),
                                });
                            }
                            else if (const auto rule = recurrent_rule_type[keywords::RECURRENT_INFIX]) {
                                nested.push_back(RecInfix{
                                    .pattern = rule[keywords::RECURRENT_INFIX_PATTERN].as<std::string>(),
                                });
                            }
                        }

                        return nested;
                    }

                    std::unreachable();
                }(),
                .serialization_script = as_opt<details::yaml::Script>(tag[keywords::SERIALIZATION_SCRIPT]),
                .deserialization_script = as_opt<details::yaml::Script>(tag[keywords::DESERIALIZATION_SCRIPT]),
            };
        }

        // FIXME logic validation (e.g. claim prefix if several existing of one tag)

        return result;
    }
    catch ([[maybe_unused]] std::exception& e) {
// debug print FIXME proper error handling
#ifdef _DEBUG
        std::cerr << std::format("{} error: {}", __FUNCTION__, e.what()) << std::endl;
#endif
    }
    return std::nullopt;
}
