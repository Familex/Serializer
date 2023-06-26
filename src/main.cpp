#define LUWRA_REGISTRY_PREFIX "DynSer#"

#include "dynser.h"
#include "misc.hpp"

#include <cassert>
#include <format>
#include <iostream>
#include <regex>

template <typename... Fs>
struct Overload : Fs...
{
    using Fs::operator()...;
};

int main()
{
    if constexpr (false) {
        struct Bar
        {
            bool is_left;

            auto operator<=>(const Bar&) const = default;
        } bar{ false };

        struct Foo
        {
            Bar bar;
            std::uint32_t dot;
            std::uint32_t dyn;

            auto operator<=>(const Foo&) const = default;
        } foo{ bar, 425, 1928 };

        struct Baz
        {
            std::string letter;

            auto operator<=>(const Baz&) const = default;
        } baz{ "aaa" };

        struct Pos
        {
            std::uint32_t x;
            std::uint32_t y;

            auto operator<=>(const Pos&) const = default;
        } from{ 10, 531 }, to{ 121, 4929 };

        struct Input
        {
            Pos from;
            Pos to;

            auto operator<=>(const Input&) const = default;
        } input{ from, to };

        const auto bar_to_prop = [](dynser::Context&, const Bar& target) {
            return dynser::Properties{ { "is-left", { target.is_left } } };
        };

        const auto pos_to_prop = [](dynser::Context&, const Pos& target) {
            return dynser::Properties{ { "x", { target.x } }, { "y", { target.y } } };
        };

        const auto prop_to_bar = [](dynser::Context&, dynser::Properties&& props, Bar& out) {
            out = { props["is-left"].as_bool() };
        };

        const auto prop_to_pos = [](dynser::Context&, dynser::Properties&& props, Pos& out) {
            out = { any_cast<std::uint32_t>(props["x"].data), any_cast<std::uint32_t>(props["y"].data) };
        };

        dynser::DynSer ser{
            dynser::PropertyToTargetMapper{
                prop_to_bar,
                [&prop_to_bar](dynser::Context& ctx, dynser::Properties&& props, Foo& out) {
                    Bar bar;
                    prop_to_bar(ctx, dynser::util::remove_prefix(props, "bar"), bar);
                    out = { bar,
                            std::any_cast<std::uint32_t>(props["dot-stopped"].data),
                            std::any_cast<std::uint32_t>(props["len-stopped"].data) };
                },
                [](dynser::Context&, dynser::Properties&& props, Baz& out) { out = { props["letter"].as_string() }; },
                prop_to_pos,
                [&prop_to_pos](dynser::Context& ctx, dynser::Properties&& props, Input& out) {
                    Pos from, to;
                    prop_to_pos(ctx, dynser::util::remove_prefix(props, "from"), from);
                    prop_to_pos(ctx, dynser::util::remove_prefix(props, "to"), to);
                    out = { from, to };
                },
            },
            dynser::TargetToPropertyMapper{
                bar_to_prop,
                [&bar_to_prop](dynser::Context& ctx, const Foo& target) {
                    return dynser::util::add_prefix(bar_to_prop(ctx, target.bar), "bar") + dynser::Properties{
                        { "dot-stopped", { target.dot } },
                        { "len-stopped", { target.dyn } },
                    };
                },
                [](dynser::Context&, const Baz& target) {
                    return dynser::Properties{ { "letter", { target.letter } } };
                },
                pos_to_prop,
                [&pos_to_prop](dynser::Context& ctx, const Input& target) {
                    return dynser::util::add_prefix(pos_to_prop(ctx, target.from), "from") +
                           dynser::util::add_prefix(pos_to_prop(ctx, target.to), "to");
                },
            },
            dynser::config::FileName{ "./yaml/example2.yaml" }
        };

        const auto round_trip = [&ser](const auto& target, const std::string_view tag) noexcept {
            using T = std::remove_cvref_t<decltype(target)>;
            return target == ser.deserialize<T>(ser.serialize(target, tag), tag);
        };

        ser.context["val-length"] = { 4 };
        ser.context["type"] = { "a" };

        const auto res = round_trip(foo, "foo") && round_trip(bar, "bar") && round_trip(baz, "baz") &&
                         round_trip(from, "pos") && round_trip(input, "input");
    }

    if constexpr (false) {
        using namespace dynser::config::details;

        const auto dyn_regex = yaml::DynRegex{ R"(\_2\[\d{\_1},\d{\_1}\])" };
        const auto without_prefix = resolve_dyn_regex(clone(dyn_regex), yaml::DynGroupValues{ { 1, "4" } });
        const auto res0 = std::regex_match("[0010,1999]", std::regex{ without_prefix.value });

        const auto with_prefix =
            resolve_dyn_regex(clone(dyn_regex), yaml::DynGroupValues{ { 1, "2" }, { 2, "pos: " } });
        const auto res1 = std::regex_match("pos: [04,12]", std::regex{ with_prefix.value });
        const auto res2 = std::regex_match("pos:[04,12]", std::regex{ with_prefix.value });
        const auto res3 = std::regex_match("pos: [4,12]", std::regex{ with_prefix.value });

        assert(res0 && res1 && !res2 && !res3);
    }

    if constexpr (false) {
        using namespace dynser::config::details::regex;

        /*
        const std::optional<Quantifier> test[]{ search_quantifier("?[abs]{3,4}"), search_quantifier("*[futnntusrnay"),
                                                search_quantifier("{1,2}"),       search_quantifier("{1}"),
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

    if constexpr (true) {
        using namespace dynser::config::details::regex;
        using dynser::config::details::yaml::GroupValues;
        using Test = std::pair<std::string, GroupValues>;

        Test tests[]{
            { "(\\d{2})", { { 1, "999" } } },
            { "(\\d+){2}", { { 1, "10" } } },
            { "(\\d{2})", { { 1, "99" } } },
            { "(\\d{2})", { { 1, "099" } } },
            { "(.+)/(.+)", { { 1, "left" }, { 2, "right" } } },
        };

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
}
