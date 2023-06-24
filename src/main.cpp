#define LUWRA_REGISTRY_PREFIX "DynSer#"

#include "dynser.h"

int main()
{
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
        dynser::ConfigFileName{ "./yaml/example2.yaml" }
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
