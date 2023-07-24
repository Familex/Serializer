#pragma once

#include "dynser.h"
#include "printer.hpp"
#include <catch2/catch_test_macros.hpp>

namespace dynser_test
{
struct Bar
{
    bool is_left;

    auto operator<=>(const Bar&) const = default;
};

struct Foo
{
    Bar bar;
    std::int32_t dot;
    std::int32_t dyn;

    auto operator<=>(const Foo&) const = default;
};

struct Baz
{
    std::string letter;

    auto operator<=>(const Baz&) const = default;
};

struct Pos
{
    std::int32_t x;
    std::int32_t y;

    auto operator<=>(const Pos&) const = default;
};

struct Input
{
    Pos from;
    Pos to;

    auto operator<=>(const Input&) const = default;
};

auto get_dynser_instance() noexcept
{
    auto const bar_to_prop = [](dynser::Context&, const Bar& target) noexcept {
        return dynser::Properties{ { "is-left", dynser::PropertyValue{ target.is_left } } };
    };

    auto const pos_to_prop = [](dynser::Context&, const Pos& target) noexcept {
        return dynser::Properties{ { "x", dynser::PropertyValue{ target.x } },
                                   { "y", dynser::PropertyValue{ target.y } } };
    };

    auto const prop_to_bar = [](dynser::Context&, dynser::Properties&& props, Bar& out) noexcept {
        out = { props["is-left"].as_bool() };
    };

    auto const prop_to_pos = [](dynser::Context&, dynser::Properties&& props, Pos& out) noexcept {
        out = { props["x"].as_i32(), props["y"].as_i32() };
    };

    static dynser::DynSer result{
        // deserialization
        dynser::PropertyToTargetMapper{
            prop_to_bar,
            [&](dynser::Context& ctx, dynser::Properties&& props, Foo& out) {
                Bar bar;
                prop_to_bar(ctx, dynser::Properties{ props }, bar);
                out = { bar, props["dot-stopped"].as_i32(), props["len-stopped"].as_i32() };
            },
            [&](dynser::Context&, dynser::Properties&& props, Baz& out) { out = { props["letter"].as_string() }; },
            prop_to_pos,
            [&](dynser::Context& ctx, dynser::Properties&& props, Input& out) {
                Pos from, to;
                prop_to_pos(ctx, dynser::util::remove_prefix(props, "from"), from);
                prop_to_pos(ctx, dynser::util::remove_prefix(props, "to"), to);
                out = { from, to };
            },
            [&](this auto const& self, dynser::Context& ctx, dynser::Properties&& props, std::vector<int>& out) {
                auto last_element = props["last-element"].as_i32();
                while (props.contains("element")) {
                    out.push_back(props["element"].as_i32());
                    props = dynser::util::remove_prefix(props, "next");
                }
                out.push_back(last_element);
            },
            [&](dynser::Context&, dynser::Properties&& props, std::vector<Pos>& out) {
                std::unreachable();    // not implemented
            } },
        // serialization
        dynser::TargetToPropertyMapper{
            bar_to_prop,
            [&](dynser::Context& ctx, const Foo& target) {
                return bar_to_prop(ctx, target.bar) + dynser::Properties{
                    { "dot-stopped", dynser::PropertyValue{ target.dot } },
                    { "len-stopped", dynser::PropertyValue{ target.dyn } },
                };
            },
            [&](dynser::Context&, const Baz& target) {
                return dynser::Properties{ { "letter", dynser::PropertyValue{ target.letter } } };
            },
            pos_to_prop,
            [&](dynser::Context& ctx, const Input& target) {
                return dynser::util::add_prefix(pos_to_prop(ctx, target.from), "from") +    // clang-format comment
                       dynser::util::add_prefix(pos_to_prop(ctx, target.to), "to");
            },
            [&](dynser::Context& ctx, const std::vector<int>& target) {
                dynser::Properties result;

                if (target.empty()) {
                    return result;
                }

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
            [&](dynser::Context&, const std::vector<Pos>& target) -> dynser::Properties {
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

    return result;
}

#define DYNSER_TEST_SERIALIZE(target, tag, expected)                                                                   \
    do {                                                                                                               \
        using namespace dynser_test;                                                                                   \
        const auto serialized = ser.serialize(target, tag);                                                            \
        if (!serialized) {                                                                                             \
            UNSCOPED_INFO("Serialize error: " << Printer{}.serialize_err_to_string(serialized.error()));               \
        }                                                                                                              \
        REQUIRE(serialized);                                                                                           \
        CHECK(*serialized == expected);                                                                                \
    } while (false);

#define DYNSER_TEST_SERIALIZE_CTX(target, tag, expected, context)                                                      \
    do {                                                                                                               \
        using namespace dynser_test;                                                                                   \
        ser.context = context;                                                                                         \
        const auto serialized = ser.serialize(target, tag);                                                            \
        if (!serialized) {                                                                                             \
            UNSCOPED_INFO("Serialize error: " << Printer{}.serialize_err_to_string(serialized.error()));               \
        }                                                                                                              \
        REQUIRE(serialized);                                                                                           \
        CHECK(*serialized == expected);                                                                                \
        ser.context.clear();                                                                                           \
    } while (false);

}    // namespace dynser_test
