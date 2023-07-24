#include "common.hpp"

TEST_CASE("Recurrent rule")
{
    using namespace dynser_test;

    const auto config = R"##(---
version: ''
tags:
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
...)##";

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

    SECTION("'Pos-list' rule", "[recurrent]")
    {
        std::pair<std::vector<Pos>, std::string> lists[]{
            { { { 1, 2 }, { 3, 4 } }, "[ ( 1, 2 ), ( 3, 4 ) ]" },
            { { { -1, -254 }, { 123, 0 }, { 00, 2 } }, "[ ( -1, -254 ), ( 123, 0 ), ( 0, 2 ) ]" },
            { {}, "[  ]" },
            { { {} }, "[ ( 0, 0 ) ]" },
        };

        for (auto const& [list, expected] : lists) {
            DYNSER_TEST_SERIALIZE(list, "pos-list", expected);
        }
    }
}
