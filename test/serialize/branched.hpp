#include "common.hpp"

TEST_CASE("Branched rule")
{
    using namespace dynser_test;

    const auto config = R"##(---
# yaml-language-server: $schema=yaml/schema_2.json
version: ''
tags:
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
...)##";

    auto ser = get_dynser_instance();

    const auto result = ser.load_config(dynser::config::RawContents{ config });

    INFO("Config: " << config);
    REQUIRE(result);

    SECTION("'Bar' rule", "[branched] [linear]")
    {

        std::pair<Bar, std::string> bars[]{
            { { false }, "right" },
            { { true }, "left" },
        };

        for (auto const& [bar, expected] : bars) {
            DYNSER_TEST_SERIALIZE(bar, "bar", expected);
        }
    }

    SECTION("'Baz' rule", "[branched] [linear] [existing] [helper] [context]")
    {
        using Prop = dynser::PropertyValue;

        std::tuple<Baz, dynser::Context, std::string> bazs[]{
            { { "aaaa" }, { { "type", Prop{ "a" } } }, "aaaa" },
            { { "bbbb" }, { { "type", Prop{ "b" } } }, "bbbb" },
            { { "" }, { { "type", Prop{ "a" } } }, "" },
        };

        for (auto const& [baz, context, expected] : bazs) {
            DYNSER_TEST_SERIALIZE_CTX(baz, "baz", expected, context);
        }
    }
}
