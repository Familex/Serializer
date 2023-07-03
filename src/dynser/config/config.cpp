#include "config.h"

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
    try {
        const auto yaml = YAML::Load(std::string{ sv });
        Config result;

        result.version = yaml["version"].as<std::string>();

        for (const auto tag : yaml["tags"]) {
            const auto tag_name = tag["name"].as<std::string>();

            result.tags[tag_name] = {
                .name = tag_name,
                .nested = [&]() -> details::yaml::Nested {    // iife
                    if (const auto continual = tag["continual"]) {
                        std::vector<details::yaml::Continual> result;

                        for (const auto continual_type : continual) {
                            if (const auto nested = continual_type["existing"]) {
                                result.push_back(details::yaml::Existing{ .tag = nested["tag"].as<std::string>(),
                                                                          .prefix =
                                                                              as_opt<std::string>(nested["prefix"]) });
                            }
                            else if (const auto nested = continual_type["linear"]) {
                                result.push_back(details::yaml::Linear{
                                    .pattern = nested["pattern"].as<std::string>(),
                                    .dyn_groups = as_opt<details::yaml::DynGroupValues>(nested["dyn-groups"]),
                                    .fields = as_opt<details::yaml::GroupValues>(nested["fields"]) });
                            }
                            else if (const auto nested = continual_type["branched"]) {
                                const auto type{ nested["type"].as<std::string>() };
                                if (type == "match-successfulness") {
                                    result.push_back(details::yaml::Branched{
                                        details::yaml::BranchedMatchSuccessfulness{
                                            .patterns = nested["patterns"].as<std::vector<details::yaml::Regex>>(),
                                            .fields = as_opt<details::yaml::GroupValues>(nested["fields"]) } });
                                }
                                else if (type == "script-variable") {
                                    result.push_back(details::yaml::Branched{ details::yaml::BranchedScriptVariable{
                                        .variable = nested["variable"].as<std::string>(),
                                        .script = nested["script"].as<std::string>(),
                                        .patterns =
                                            nested["patterns"].as<details::yaml::BranchedScriptVariable::Patterns>(),
                                        .fields = as_opt<details::yaml::GroupValues>(nested["fields"]) } });
                                }
                            }
                        }

                        return result;
                    }
                    else if (const auto recurrent = tag["recurrent"]) {
                        std::vector<details::yaml::Recurrent> result;

                        result.push_back(details::yaml::Recurrent{});

                        return result;
                    }
                }(),
                .serialization_script = as_opt<details::yaml::Script>(tag["serialization-script"]),
                .deserialization_script = as_opt<details::yaml::Script>(tag["deserialization-script"])
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
