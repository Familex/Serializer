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

inline auto const bar_to_prop = [](dynser::Context&, const Bar& target) noexcept {
    return dynser::Properties{ { "is-left", dynser::PropertyValue{ target.is_left } } };
};

inline auto const pos_to_prop = [](dynser::Context&, const Pos& target) noexcept {
    return dynser::Properties{ { "x", dynser::PropertyValue{ target.x } }, { "y", dynser::PropertyValue{ target.y } } };
};

inline auto const prop_to_bar = [](dynser::Context&, dynser::Properties&& props, Bar& out) noexcept {
    out = { props["is-left"].as_bool() };
};

inline auto const prop_to_pos = [](dynser::Context&, dynser::Properties&& props, Pos& out) noexcept {
    out = { props["x"].as_i32(), props["y"].as_i32() };
};

inline dynser::DynSer ser{
    // deserialization
    dynser::PropertyToTargetMapper{
        prop_to_bar,
        [](dynser::Context& ctx, dynser::Properties&& props, Foo& out) {
            Bar bar;
            prop_to_bar(ctx, dynser::Properties{ props }, bar);
            out = { bar, props["dot-stopped"].as_i32(), props["len-stopped"].as_i32() };
        },
        [](dynser::Context&, dynser::Properties&& props, Baz& out) { out = { props["letter"].as_string() }; },
        prop_to_pos,
        [](dynser::Context& ctx, dynser::Properties&& props, Input& out) {
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
        [](dynser::Context& ctx, const Foo& target) {
            return bar_to_prop(ctx, target.bar) + dynser::Properties{
                { "dot-stopped", dynser::PropertyValue{ target.dot } },
                { "len-stopped", dynser::PropertyValue{ target.dyn } },
            };
        },
        [](dynser::Context&, const Baz& target) {
            return dynser::Properties{ { "letter", dynser::PropertyValue{ target.letter } } };
        },
        pos_to_prop,
        [](dynser::Context& ctx, const Input& target) {
            return dynser::util::add_prefix(pos_to_prop(ctx, target.from), "from") +    // clang-format comment
                   dynser::util::add_prefix(pos_to_prop(ctx, target.to), "to");
        },
        [](dynser::Context& ctx, const std::vector<int>& target) {
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

struct LoadDynSerConfig
{
    bool load_result{ false };
    LoadDynSerConfig()
    {
        const char* const common_dynser_config{
            R"CFG(---
# yaml-language-server: $schema=./schema_2.json
version: 'example2'
tags:
  - name: "foo"
    continual:
      - existing: { tag: "bar" }
      - linear:
          pattern: '\.(-?\d+)\.'
          fields:
            1: dot-stopped
      - linear:
          pattern: '-?\d{\_1}'
          dyn-groups:
            1: val-length
          fields:
            0: len-stopped
    serialization-script: |
      out['dot-stopped'] = tostring(inp['dot-stopped']:as_i32())
      local len_stopped_str = tostring(inp['len-stopped']:as_i32())
      local is_neg = inp['len-stopped']:as_i32() < 0
      local len = tonumber(ctx['val-length']:as_string())
      while len_stopped_str:len() < len + (is_neg and 1 or 0) do
        if is_neg then
          -- add '0' after minus sign
          len_stopped_str = len_stopped_str:sub(1, 1) .. '0' .. len_stopped_str:sub(2)
        else
          len_stopped_str = '0' .. len_stopped_str
        end
      end
      out['len-stopped'] = len_stopped_str
    deserialization-script: |
      out['dot-stopped'] = tonumber(inp['dot-stopped'])
      out['len-stopped'] = tonumber(inp['len-stopped'])

  - name: "bar"
    branched:
      branching-script: |
         branch = inp['is-left']:as_bool() and 0 or 1 
      debranching-script: |
         out['is-left'] = branch == 1
      rules:
        - linear: { pattern: 'left' }
        - linear: { pattern: 'right' }

  - name: "baz"
    branched:
      branching-script: |
        -- error without round brackets
        branch = ({ a = 0, b = 1 })[ctx['type']:as_string()]
      debranching-script: |
        ctx['type'] = ({ 0 = 'a', 1 = 'b' })[branch]
      rules:
        # TODO inline (anonymous) tags
        - existing: { tag: "baz-helper-a" }
        - existing: { tag: "baz-helper-b" }
  - name: "baz-helper-a"
    continual:
      - linear: { pattern: "a*", fields: { 0: letter } }
      # TODO pass fields through without scripts
    serialization-script: |
      out['letter'] = inp['letter']:as_string()
    deserialization-script: |
      out['letter'] = inp['letter']
  - name: "baz-helper-b"
    continual:
      - linear: { pattern: "b*", fields: { 0: letter } }
    serialization-script: |
      out['letter'] = inp['letter']:as_string()
    deserialization-script: |
      out['letter'] = inp['letter']

  - name: "pos"
    continual:
      - linear:
          pattern: '(-?\d+), (-?\d+)'
          fields:
            1: x
            2: y
    serialization-script: |
      out['x'] = tostring(inp['x']:as_i32())
      out['y'] = tostring(inp['y']:as_i32())
    deserialization-script: |
      out['x'] = tonumber(inp['x'])
      out['y'] = tonumber(inp['y'])

  - name: "input"
    continual:
      - linear:
          pattern: 'from: \('
      - existing: # existing tag several times (reason of prefix existence)
          tag: "pos"
          prefix: from
      - linear:
          pattern: '\) to: \('
      - existing:
          tag: "pos"
          prefix: to
      - linear:
          pattern: '\)'

  - name: "recursive"
    continual:
      - linear: { pattern: '\[ ' }
      - existing: { tag: "recursive-payload" }
      - linear: { pattern: '-?\d+', fields: { 0: last-element } }
      - linear: { pattern: ' \]' }
    serialization-script: |
      out['last-element'] = tostring(inp['last-element']:as_i32())
    deserialization-script: |
      out['last-element'] = tonumber(inp['last-element'])
  - name: "recursive-payload"
    continual:
      - linear: { pattern: '(-?\d+), ', fields: { 1: element } }
      - existing: { tag: "recursive-payload", prefix: "next", required: false }
    serialization-script: |
      if inp['element'] ~= nil then
        out['element'] = tostring(inp['element']:as_i32())
      end
      -- else do nothing and trigger check in c++ code
    deserialization-script: |
      -- recursion exit?
      out['element'] = tonumber(inp['element'])

  - name: "pos-list"
    continual:
      - linear: { pattern: '\[ ' }
      - existing: { tag: "pos-list-payload" }
      - linear: { pattern: ' \]' }
  - name: "pos-list-payload"
    recurrent:
      - linear: { pattern: '\( ' }
      - existing: { tag: "pos" }
      - linear: { pattern: ' \)' }
      - infix: { pattern: ', ' }

  - name: "pos-list-list"
    continual:
      - linear: { pattern: '\[ ' }
      - existing: { tag: "pos-list-list-payload" }
      - linear: { pattern: ' \]' }
  - name: "pos-list-list-payload"
    recurrent:
      - existing: { tag: "pos-list", priority: 2 }
      - infix: { pattern: ', ' }

  - name: "existing-existing-test"
    continual:
      - linear: { pattern: 'exex: ' }
      - existing: { tag: "existing-test" }
  - name: "existing-test"
    continual:
      - linear: { pattern: 'ex: ' }
      - existing: { tag: "existing-test-payload" }
  - name: "existing-test-payload"
    continual:
      - linear: { pattern: 'existing-test-data' }
...
)CFG"
        };

        load_result = ser.load_config(dynser::config::RawContents{ common_dynser_config });
    }
} inline const trigger_load_dynser_config{};

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
