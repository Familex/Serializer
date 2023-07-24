#pragma once

#include "dynser.h"
#include <format>

namespace dynser_test
{

template <typename... Fs>
struct Overload : Fs...
{
    using Fs::operator()...;
};

struct Printer
{
    std::string serialize_err_to_string(dynser::SerializeError const& wrapper) noexcept
    {
        using namespace dynser::serialize_err;

        std::string ref_str;

        for (const auto& el : wrapper.ref_seq) {
            ref_str += std::format(" from '{}' tag at '{}' rule", el.tag, el.rule_ind);
        }

        auto error_str = std::visit(
            Overload{
                [](const Unknown&) -> std::string { return "unknown error"; },
                [](const ConfigNotLoaded&) -> std::string { return "config not loaded"; },
                [](const ConfigTagNotFound& error) -> std::string {
                    return std::format("config tag '{}' not found", error.tag);
                },
                [](const BranchNotSet&) -> std::string { return "branch not set"; },
                [](const BranchOutOfBounds& error) -> std::string {
                    return std::format(
                        "seleced branch: '{}'; expected between 0 and '{}'", error.selected_branch, error.max_branch
                    );
                },
                [](const ScriptError& error) -> std::string {
                    return std::format("lua script error: '{}'", error.message);
                },
                [](const ScriptVariableNotFound& error) {
                    return std::format("script variable '{}' not set", error.variable_name);
                },
                [](const ResolveRegexError& error) -> std::string {
                    using namespace dynser::regex::to_string_err;

                    return std::format(
                        "regex error '{}' on group '{}'",
                        std::visit(
                            Overload{
                                [](const RegexSyntaxError& reg_err) -> std::string {
                                    return std::format("syntax error at {}", reg_err.position);
                                },
                                [](const MissingValue&) -> std::string { return "missing value"; },
                                [](const InvalidValue& reg_err) -> std::string {
                                    return std::format("invalid value: '{}'", reg_err.value);
                                },
                            },
                            error.error.error
                        ),
                        error.error.group_num
                    );
                } },
            wrapper.error
        );

        return error_str + ref_str;
    }

    std::string config_parse_err_to_string(dynser::config::ParseError const& error) noexcept
    {
        using enum dynser::config::ParseError::Type;

        switch (error.type) {
            case ParserException:
                return std::format(
                    "yaml-cpp parser exception at 'pos: {}, line: {}, col: {}' with msg: {}",
                    error.mark.pos,
                    error.mark.line,
                    error.mark.column,
                    error.msg
                );
            case RepresentationException:
                return std::format(
                    "yaml-cpp representation exception at 'pos: {}, line: {}, col: {}' with msg: {}",
                    error.mark.pos,
                    error.mark.line,
                    error.mark.column,
                    error.msg
                );
            case UnknownYamlCppException:
                return std::format(
                    "yaml-cpp unknown exception at 'pos: {}, line: {}, col: {}' with msg: {}",
                    error.mark.pos,
                    error.mark.line,
                    error.mark.column,
                    error.msg
                );
            case UnknownException:
                return std::format("unknown exception");
        }
        std::unreachable();
    }
};

}    // namespace dynser_test
