#define LUWRA_REGISTRY_PREFIX "DynSer#"

#include "dynser.h"
#include "misc.hpp"

#include <cassert>
#include <format>
#include <iostream>
#include <ranges>
#include <regex>

template <typename... Fs>
struct Overload : Fs...
{
    using Fs::operator()...;
};

int main()
{
    if constexpr (true) {
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
                    Overload{ [](const Unknown&) -> std::string { return "unknown error"; },
                              [](const ConfigNotLoaded&) -> std::string { return "config not loaded"; },
                              [](const BranchNotSet&) -> std::string { return "branch not set"; },
                              [](const BranchOutOfBounds& error) -> std::string {
                                  return std::format(
                                      "seleced branch: '{}'; expected between 0 and '{}'",
                                      error.selected_branch,
                                      error.max_branch
                                  );
                              },
                              [](const ScriptError& error) -> std::string {
                                  return std::format("lua script error: '{}'", error.message);
                              },
                              [](const ScriptVariableNotFound& error) {
                                  return std::format("script variable '{}' not set", error.variable_name);
                              },
                              [](const ResolveRegexError& error) -> std::string {
                                  using namespace dynser::config::details::regex::to_string_err;

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
        } printer{};
        struct Bar
        {
            bool is_left;

            auto operator<=>(const Bar&) const = default;
        } bar{ false };

        struct Foo
        {
            Bar bar;
            std::int32_t dot;
            std::int32_t dyn;

            auto operator<=>(const Foo&) const = default;
        } foo{ bar, 425, 1928 };

        struct Baz
        {
            std::string letter;

            auto operator<=>(const Baz&) const = default;
        } baz{ "aaa" };

        struct Pos
        {
            std::int32_t x;
            std::int32_t y;

            auto operator<=>(const Pos&) const = default;
        } from{ 10, 531 }, to{ 121, 4929 };

        struct Input
        {
            Pos from;
            Pos to;

            auto operator<=>(const Input&) const = default;
        } input{ from, to };
        /*
            from-x
            from-y
            to-x
            to-y
        */

        const auto bar_to_prop = [](dynser::Context&, const Bar& target) {
            return dynser::Properties{ { "is-left", dynser::PropertyValue{ target.is_left } } };
        };

        const auto pos_to_prop = [](dynser::Context&, const Pos& target) {
            return dynser::Properties{ { "x", dynser::PropertyValue{ target.x } },
                                       { "y", dynser::PropertyValue{ target.y } } };
        };

        const auto prop_to_bar = [](dynser::Context&, dynser::Properties&& props, Bar& out) {
            out = { props["is-left"].as_bool() };
        };

        const auto prop_to_pos = [](dynser::Context&, dynser::Properties&& props, Pos& out) {
            out = { props["x"].as_i32(), props["y"].as_i32() };
        };

        dynser::DynSer ser{
            // deserialization
            dynser::PropertyToTargetMapper{
                prop_to_bar,
                [&prop_to_bar](dynser::Context& ctx, dynser::Properties&& props, Foo& out) {
                    Bar bar;
                    prop_to_bar(ctx, dynser::Properties{ props }, bar);
                    out = { bar, props["dot-stopped"].as_i32(), props["len-stopped"].as_i32() };
                },
                [](dynser::Context&, dynser::Properties&& props, Baz& out) { out = { props["letter"].as_string() }; },
                prop_to_pos,
                [&prop_to_pos](dynser::Context& ctx, dynser::Properties&& props, Input& out) {
                    Pos from, to;
                    prop_to_pos(ctx, dynser::util::remove_prefix(props, "from"), from);
                    prop_to_pos(ctx, dynser::util::remove_prefix(props, "to"), to);
                    out = { from, to };
                },
                [](this auto const& self, dynser::Context& ctx, dynser::Properties&& props, std::vector<int>& out) {
                    auto last_element = props["last-element"].as_i32();
                    while (props.contains("element")) {
                        out.push_back(props["element"].as_i32());
                        props = dynser::util::remove_prefix(props, "next");
                    }
                    out.push_back(last_element);
                },
                [](dynser::Context&, dynser::Properties&& props, std::vector<Pos>& out) {
                    std::unreachable();    // not implemented
                } },
            // serialization
            dynser::TargetToPropertyMapper{
                bar_to_prop,
                [&bar_to_prop](dynser::Context& ctx, const Foo& target) {
                    return bar_to_prop(ctx, target.bar) + dynser::Properties{
                        { "dot-stopped", dynser::PropertyValue{ target.dot } },
                        { "len-stopped", dynser::PropertyValue{ target.dyn } },
                    };
                },
                [](dynser::Context&, const Baz& target) {
                    return dynser::Properties{ { "letter", dynser::PropertyValue{ target.letter } } };
                },
                pos_to_prop,
                [&pos_to_prop](dynser::Context& ctx, const Input& target) {
                    return dynser::util::add_prefix(pos_to_prop(ctx, target.from), "from") +    // clang-format comment
                           dynser::util::add_prefix(pos_to_prop(ctx, target.to), "to");
                },
                [](dynser::Context& ctx, const std::vector<int>& target) {
                    dynser::Properties result;

                    for (std::size_t cnt{}, ind{}; ind < target.size() - 1; ++ind) {
                        auto val = dynser::PropertyValue{ target[ind] };
                        dynser::Properties val_prop{ { "element", val } };
                        for (std::size_t i{}; i < cnt; ++i) {
                            val_prop = dynser::util::add_prefix(val_prop, "next");
                        }
                        result.merge(val_prop);
                        ++cnt;
                    }

                    result["last-element"] = dynser::PropertyValue{ target.back() };

                    return result;
                },
                [](dynser::Context&, const std::vector<Pos>& target) -> dynser::Properties {
                    using List = dynser::PropertyValue::ListType<dynser::PropertyValue>;

                    dynser::Properties result;

                    result["x"] = dynser::PropertyValue{ List{} };
                    result["y"] = dynser::PropertyValue{ List{} };
                    for (const auto& el : target) {
                        result["x"].as_list().push_back(dynser::PropertyValue{ el.x });
                        result["y"].as_list().push_back(dynser::PropertyValue{ el.y });
                    }

                    return result;
                },
            }
        };
        const auto load_result = ser.load_config(dynser::config::FileName{ "./yaml/example2.yaml" });

        const auto round_trip = [&ser, &printer](const auto& target, const std::string_view tag) noexcept {
            using T = std::remove_cvref_t<decltype(target)>;
            const auto serialize_result = ser.serialize(target, tag);
            std::cout << std::format("serialized '{}' is '{}'\n", tag, serialize_result ? *serialize_result : "none");
            if (!serialize_result) {
                std::cout << std::format(
                    "Serialize error of '{}': {}", tag, printer.serialize_err_to_string(serialize_result.error())
                );
                return false;
            }
            const auto deserialize_result = ser.deserialize<T>(*serialize_result, tag);
            if (!deserialize_result) {
                return false;
            }
            return target == *deserialize_result;
        };

        const auto serialize_props_print = [&ser, &printer](const auto& props, const std::string_view tag) noexcept {
            const auto serialize_result = ser.serialize_props(props, tag);
            std::cout << std::format("serialized '{}' is '{}'\n", tag, serialize_result ? *serialize_result : "none");
            if (!serialize_result) {
                std::cout << std::format(
                    "Serialize error of '{}': {}", tag, printer.serialize_err_to_string(serialize_result.error())
                );
                return false;
            }
            return true;
        };

        ser.context["val-length"] = dynser::PropertyValue{ "4" };
        ser.context["type"] = dynser::PropertyValue{ "a" };

        std::size_t res{};
        // res += serialize_props_print(dynser::Properties{}, "existing-existing-test");
        // res += round_trip(foo, "foo");
        // res += round_trip(from, "pos");
        // res += round_trip(input, "input");
        // res += round_trip(bar, "bar");
        // res += round_trip(baz, "baz");
        // res += round_trip(std::vector<int>{ 14, 15, 16, 1034, -1249, 0, 0, -12948 }, "recursive");
        res += round_trip(std::vector<Pos>{ { 1, 2 }, { 3, 4 } }, "pos-list");
    }

    if constexpr (false) {
        using namespace dynser::config::details;

        const auto dyn_regex = yaml::DynRegex{ R"(\_2\[\d{\_1},\d{\_1}\])" };
        const auto without_prefix = resolve_dyn_regex(clone(dyn_regex), yaml::DynGroupValues{ { 1, "4" } });
        const auto res0 = std::regex_match("[0010,1999]", std::regex{ without_prefix });

        const auto with_prefix =
            resolve_dyn_regex(clone(dyn_regex), yaml::DynGroupValues{ { 1, "2" }, { 2, "pos: " } });
        const auto res1 = std::regex_match("pos: [04,12]", std::regex{ with_prefix });
        const auto res2 = std::regex_match("pos:[04,12]", std::regex{ with_prefix });
        const auto res3 = std::regex_match("pos: [4,12]", std::regex{ with_prefix });

        assert(res0 && res1 && !res2 && !res3);
    }

    if constexpr (false) {
        using namespace dynser::config::details::regex;

        /*
        const std::optional<Quantifier> test[]{ search_quantifier("?[abs]{3,4}"),
        search_quantifier("*[futnntusrnay"), search_quantifier("{1,2}"),       search_quantifier("{1}"),
                                                search_quantifier("{1,}"),        search_quantifier("{1}?"),
                                                search_quantifier("{10,}?"),      search_quantifier("+?ft") };
                                                */
        const auto s = 2;
    }

    if constexpr (false) {
        using namespace dynser::config::details::regex;

        // make print fuctions what referring to each other in this scope
        struct Printer
        {
            auto quantifier_as_str(Quantifier const& q) const -> std::string
            {
                if (q == without_quantifier) {
                    return std::format("'without quantifier'");
                }
                if (q.to) {
                    return std::format("{{ from: {}, to: {}, is_lazy: {} }}", q.from, *q.to, q.is_lazy);
                }
                else {
                    return std::format("{{ from: {}, is_lazy: {} }}", q.from, q.is_lazy);
                }
            }
            auto token_as_str(Token const& token) const -> std::string
            {
                return std::visit(
                    Overload{
                        [&](const Empty&) { return std::format("{{ type: empty }}"); },
                        [&](const WildCard& w) {
                            return std::format("{{ type: wildcard, quantifier: {} }}", quantifier_as_str(w.quantifier));
                        },
                        [&](const Group& gr) {
                            return std::format(
                                "{{ type: group, is_capturing: {}, quantifier: {}, number: {}, value: {} }}",
                                gr.is_capturing,
                                quantifier_as_str(gr.quantifier),
                                gr.number,
                                regex_as_str(*gr.value)
                            );
                        },
                        [&](const Backreference& b) {
                            return std::format(
                                "{{ type: backreference, group_number: {}, quantifier: {} }}",
                                b.group_number,
                                quantifier_as_str(b.quantifier)
                            );
                        },
                        [&](const Lookup& l) {
                            return std::format(
                                "{{ type: lookup, is_negative: {}, is_forward: {}, value: {} }}",
                                l.is_negative,
                                l.is_forward,
                                regex_as_str(*l.value)
                            );
                        },
                        [&](const CharacterClass& c) {
                            return std::format(
                                "{{ type: character-class, is_negative: {}, qantifier: {}, characters: '{}' }}",
                                c.is_negative,
                                quantifier_as_str(c.quantifier),
                                c.characters
                            );
                        },
                        [&](const Disjunction& d) {
                            return std::format(
                                "{{ type: disjunction, left: {}, right: {} }}",
                                token_as_str(*d.left),
                                token_as_str(*d.right)
                            );
                        } },
                    token
                );
            }

            auto regex_as_str(Regex const& regex) const -> std::string
            {
                std::string result{ "[ " };
                for (const auto& el : regex.value) {
                    result += token_as_str(el);
                    result += ", ";
                }
                result.pop_back();
                result.pop_back();
                return result + " ]";
            }
        } p;

        const std::expected<Regex, std::size_t> rs[]{
            from_string("(a)(b)((c)((d)))"),
            from_string("((ke(((\\.+)+){0,0})?){20,25}ab)"),
            from_string("(?=[lookahead])"),
            from_string("(?![negative]|[lookahead])"),
            from_string("(?<=[lookbehind]*)"),
            from_string("(?<![negative][ ][lookbehind]?)"),
            from_string("(\\d+)\\."),
            from_string("(\\w*).{5,256}?"),
            from_string("a|b"),
            from_string("[ab]+|b"),
            from_string("|a|b|3"),
            from_string("[astf]{2,}|futnun[^tfnu]*"),
            from_string("[astf]{2,}|futnun[^tfnu]*"),
            from_string("[\\d\\S\\]]{2,}\\10+"),
            from_string("(\\d{2,4})-(\\d{2,4})-(\\d{2,4})"),
        };
        // can be formatted as yaml list
        for (std::size_t num{}; const auto& r : rs) {
            std::cout << "- " << (r ? p.regex_as_str(*r) : std::format("'error on {} index'", r.error())) << "\n\n";
        }
    }

    if constexpr (false) {
        using namespace dynser::config::details::regex;
        using dynser::config::details::yaml::GroupValues;
        using Test = std::pair<std::string, GroupValues>;

        Test tests[]{
            { "(?:[)](\\w+)){2}", { { 2, "word" } } },                   //
            { R"((?:\)\(\[\][\]]){2,})", {} },                           //
            { "[[\\]]", {} },                                            //
            { "[\\][]", {} },                                            //
            { "(\\d{2})", { { 1, "099" } } },                            //
            { "(\\d{4,})", { { 1, "9" } } },                             //
            { "(\\w{10})!", { { 1, "ha" } } },                           //
            { "(\\d{2})", { { 1, "999" } } },                            //
            { "(\\d+){2}", { { 1, "10" } } },                            //
            { "(\\d{2})", { { 1, "99" } } },                             //
            { "(.+)[\\\\/](.+)", { { 1, "left" }, { 2, "right" } } },    //
            { "(.+)\\1{4,5}", { { 1, "go!" } } },                        //
            { " [ ", {} },                                               //
            { "[", {} },                                                 //
            { "(", {} },                                                 //
            { ")", {} },                                                 //
            { "]", {} },                                                 //
            { "", { { 1, "this group not exists" } } },                  //
        };

        // std::cout << *to_string(*from_string(test), vals) and error handle
        for (std::size_t num{}; const auto& [reg, vals] : tests) {
            std::cout << "test #" << num++ << ": ";
            const auto r = from_string(reg);
            if (!r) {
                std::cout << std::format("parse error on {} index", r.error()) << std::endl;
                continue;
            }
            const auto t = to_string(*r, vals);
            if (!t) {
                std::cout << std::format("resolve error on {} group", t.error().group_num) << std::endl;
                continue;
            }
            std::cout << "result '" << *t << "'\n";
        }
    }

    if constexpr (false) {
        using namespace dynser::config;
        using namespace dynser::details;
        using dynser::Properties;
        using PV = dynser::PropertyValue;
        using Lst = PV::ListType<PV>;

        const auto result_as_string = [](const auto& result) -> std::string {
            if (!result) {
                return "nullopt";
            }
            return std::format("priority: '{}', value: '{}'", result->first, result->second);
        };

        std::fstream config_file{ "./yaml/example2.yaml" };
        std::stringstream buffer;
        buffer << config_file.rdbuf();

        Config config{ *from_string(buffer.str()) };

        using dynser::config::details::yaml::PriorityType;

        const auto gen_result =
            [&](Properties&& props, std::string_view const tag, std::optional<PrioritizedListLen> const& expected
            ) -> std::pair<std::optional<PrioritizedListLen>, std::optional<PrioritizedListLen>> {
            return { calc_max_property_lists_len(config, std::move(props), config.tags.at(std::string{ tag }).nested),
                     expected };
        };

        std::pair<std::optional<PrioritizedListLen>, std::optional<PrioritizedListLen>> const results[]{
            gen_result(
                Properties{
                    //
                    { "x", PV{ Lst{ PV{ 1 }, PV{ 2 }, PV{ 3 } } } },
                    { "y", PV{ Lst{} } },    //
                },
                "pos-list-payload",
                { { 1, 3 } }
            ),
            gen_result(

                Properties{
                    //
                    { "x", PV{ Lst{ PV{ 1 }, PV{ 2 }, PV{ 3 } } } },
                    { "y", PV{ Lst{ PV{ 1 }, PV{ 2 }, PV{ 3 }, PV{ 4 } } } },
                },
                "pos-list-payload",
                { { 1, 4 } }
            ),
            gen_result(

                Properties{
                    //
                    { "x", PV{ Lst{ PV{ 1 }, PV{ 2 }, PV{ 3 } } } },
                    { "y", PV{ Lst{ PV{ 1 }, PV{ 2 }, PV{ 3 }, PV{ 4 } } } },
                    { "z", PV{ Lst{ PV{ 1 }, PV{ 2 }, PV{ 3 }, PV{ 4 }, PV{ 5 } } } },
                },
                "pos-list-payload",
                { { 1, 4 } }
            ),
            gen_result(

                Properties{
                    //
                    { "x", PV{ 3 } },
                    { "y", PV{ 5 } },
                },
                "pos-list-payload",
                std::nullopt
            ),
            gen_result(
                Properties{
                    //
                    { "x", PV{ 3 } },
                    { "y", PV{ Lst{ PV{ 5 } } } },
                },
                "pos-list-payload",
                { { 1, 1 } }
            ),
            gen_result(
                Properties{
                    //
                    { "x", PV{ Lst{ PV{ 1 } } } },
                    { "y", PV{ Lst{ PV{ 1 } } } },
                },
                "pos-list-list-payload",
                { { 2, 1 } }
            )
        };

        for (const auto& [result, expected] : results) {
            std::cout << std::format("res: {}; expected: {}\n", result_as_string(result), result_as_string(expected));
        }
    }
}
