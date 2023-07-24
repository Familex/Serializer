#include "common.hpp"

TEST_CASE("Recursive rule")
{
    using namespace dynser_test;

    const auto config = R"##(---
version: ''
tags:
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
...)##";

    auto ser = get_dynser_instance();

    DYNSER_LOAD_CONFIG(ser, dynser::config::RawContents{ config });

    SECTION("'recursive' rule", "[continual] [recursive] [existing] [linear] [required]")
    {
        std::pair<std::vector<int>, std::string> recursives[]{
            { { 1, 2, 3 }, "[ 1, 2, 3 ]" },
            // { {}, "[  ]" }, // not implemented
            { { -1, -3, 10, 0, 9999 }, "[ -1, -3, 10, 0, 9999 ]" },
        };

        for (auto const& [recursive, expected] : recursives) {
            DYNSER_TEST_SERIALIZE(recursive, "recursive", expected);
        }
    }
}
