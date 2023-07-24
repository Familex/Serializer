#include "common.hpp"

TEST_CASE("Continual rule")
{
    using namespace dynser_test;

    const auto config = R"##(---
# yaml-language-server: $schema=yaml/schema_2.json
version: ''
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
...)##";

    auto ser = get_dynser_instance();

    const auto result = ser.load_config(dynser::config::RawContents{ config });

    INFO("Config: " << config);
    REQUIRE(result);

    SECTION("'Foo' rule", "[continual] [existing] [linear] [dyn-groups]")
    {
        const std::pair<Foo, std::string> foos[]{
            { { { false }, 1, 2 }, "right.1.02" },
            { { { true }, -1, -3 }, "left.-1.-03" },
            { { { false }, 0, 42 }, "right.0.42" },
            { { { true }, -2, 32 }, "left.-2.32" },
        };

        ser.context["val-length"] = dynser::PropertyValue{ "2" };
        for (const auto& [foo, expected] : foos) {
            DYNSER_TEST_SERIALIZE(foo, "foo", expected);
        }
        ser.context.erase("val-length");
    }

    SECTION("'Pos' rule", "[continual] [linear]")
    {
        const std::pair<Pos, std::string> poss[]{
            { { 1, 2 }, "1, 2" },
            { { -1, -3 }, "-1, -3" },
            { { 0, 4293291 }, "0, 4293291" },
            { { -2, 328238 }, "-2, 328238" },
        };

        for (const auto& [pos, expected] : poss) {
            DYNSER_TEST_SERIALIZE(pos, "pos", expected);
        }
    }

    SECTION("'Input' rule", "[continual] [linear] [existing]")
    {
        const std::pair<Input, std::string> inputs[]    //
            { { {}, "from: (0, 0) to: (0, 0)" },        //
              { { { -1, -1 }, { 2, 2 } }, "from: (-1, -1) to: (2, 2)" } };

        for (const auto& [input, expected] : inputs) {
            DYNSER_TEST_SERIALIZE(input, "input", expected);
        }
    }
}
