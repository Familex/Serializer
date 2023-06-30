#include "config.h"

#include "yaml-cpp/yaml.h"

#include <charconv>
#include <expected>
#include <locale>
#include <optional>
#include <regex>

// debug print
#include <iostream>

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

details::yaml::Regex details::resolve_dyn_regex(yaml::DynRegex&& dyn_reg, yaml::DynGroupValues&& dyn_gr_vals) noexcept
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

details::regex::ToStringResult details::resolve_regex(yaml::Regex&& reg, yaml::GroupValues&& vals) noexcept
{
    using namespace details::regex;

    const auto reg_sus = from_string(reg);
    if (!reg_sus) {
        return std::unexpected{ ToStringError{
            ToStringErrorType::RegexSyntaxError,
            0    // group number
        } };
    }
    return to_string(*reg_sus, std::move(vals));
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

Config dynser::config::from_string(const std::string_view sv)
{

    const auto yaml = YAML::Load(std::string{ sv });
    Config result;

    result.version = yaml["version"].as<std::string>();

    for (const auto tag : yaml["tags"]) {
        const auto tag_name = tag["name"].as<std::string>();

        // clang-format off
        result.tags[tag_name] = {
            .name = tag_name,
            .nested = [&]{
                std::vector<details::yaml::Nested> result;

                for (const auto nested_type : tag["nested"]) {
                    if (const auto nested = nested_type["existing"]) {
                        result.push_back(details::yaml::Existing{
                            .tag = nested["tag"].as<std::string>(),
                            .prefix = as_opt<std::string>(nested["prefix"])
                        });
                    } else if (const auto nested = nested_type["linear"]) {
                        result.push_back(details::yaml::Linear{
                            .pattern = nested["pattern"].as<std::string>(),
                            .dyn_groups = as_opt<details::yaml::DynGroupValues>(nested["dyn-groups"]),
                            .fields = as_opt<details::yaml::GroupValues>(nested["fields"])
                        });
                    } else if (const auto nested = nested_type["branched"]) {
                        const auto type{ nested["type"].as<std::string>() };
                        if (type == "match-successfulness") {
                            result.push_back(
                                details::yaml::Branched{
                                    details::yaml::BranchedMatchSuccessfulness{
                                        .patterns = nested["patterns"].as<std::vector<details::yaml::Regex>>(),
                                        .fields = as_opt<details::yaml::GroupValues>(nested["fields"])
                                    }
                                }
                            );
                        } else if (type == "script-variable") {
                            result.push_back(
                                details::yaml::Branched{
                                    details::yaml::BranchedScriptVariable{
                                        .variable = nested["variable"].as<std::string>(),
                                        .script = nested["script"].as<std::string>(),
                                        .patterns = nested["patterns"].as<details::yaml::BranchedScriptVariable::Patterns>(),
                                        .fields = as_opt<details::yaml::GroupValues>(nested["fields"])
                                    }
                                }
                            );
                        }
                    } else if (const auto nested = nested_type["recurrent"]) {
                        result.push_back(details::yaml::Linear{
                            .pattern = nested["pattern"].as<std::string>(),
                            .dyn_groups = as_opt<details::yaml::DynGroupValues>(nested["dyn-groups"]),
                            .fields = as_opt<details::yaml::GroupValues>(nested["fields"])
                        });
                    }
                }

                return result;
            }(),
            .serialization_script = as_opt<details::yaml::Script>(tag["serialization-script"]),
            .deserialization_script = as_opt<details::yaml::Script>(tag["deserialization-script"])
        };
        // clang-format on
    }

    return result;
}
