#include "common.hpp"

TEST_CASE("Error cases")
{
    using namespace dynser;
    using namespace dynser_test;

    const auto dummy_props{ dynser::Properties{} };

    auto ser = get_dynser_instance();

    SECTION("invalid regex in config")
    {
        const auto config = R"##(---
version: ''
tags:
  - name: "1"
    continual: [ linear: { pattern: ':(' } ]
  - name: "2"
    continual: [ linear: { pattern: '+' } ]
  - name: "3"
    continual: [ linear: { pattern: ')' } ]
  - name: "4"
    continual: [ linear: { pattern: '\' } ]
...)##";

        DYNSER_LOAD_CONFIG(ser, config::RawContents{ config });

        for (auto const tag : { "1", "2", "3", "4" }) {
            DYNAMIC_SECTION("Tag: " << tag)
            {
                const auto serialize_result{ ser.serialize_props(dummy_props, tag) };

                INFO(
                    "Serialize result is: "
                    << (serialize_result ? *serialize_result
                                         : Printer{}.serialize_err_to_string(serialize_result.error()))
                );
                REQUIRE_FALSE(serialize_result);
                const auto error = serialize_result.error().error;
                REQUIRE(std::holds_alternative<serialize_err::ResolveRegexError>(error));
                const auto resolve_regex_err = std::get<serialize_err::ResolveRegexError>(error).error.error;
                REQUIRE(std::holds_alternative<regex::to_string_err::RegexSyntaxError>(resolve_regex_err));
            }
        }
    }

    SECTION("no value")
    {
        const auto config = R"##(---
version: ''
tags:
  - name: "1"
    continual: [ linear: { pattern: '.+', fields: { 0: value } } ]
  - name: "2"
    continual: [ linear: { pattern: '.+', fields: { 1: value } } ]
  - name: "3"
    continual: [ existing: { tag: "2" } ]
  - name: "4"
    branched: { branching-script: 'branch = 0', debranching-script: '', rules: [ existing: { tag: "3" } ] }
  - name: "5"
    branched: { branching-script: 'branch = 0', debranching-script: '', rules: [ existing: { tag: "4" } ] }
  - name: "6"
    branched: { branching-script: 'branch = 0', debranching-script: '', rules: [ linear: { pattern: '.+', fields: { 0: value } } ] }
# there is no 'value', but longest list is 0 length
# - name: ""
#   recurrent: [ linear: { pattern: '.+', fields: { 0: value } } ]
# - name: ""
#   recurrent: [ existing: { tag: "6" } ]
# request for two lists and not set one of them
  - name: "7"
    recurrent:
      - existing: { tag: "5" }
      - linear: { pattern: '.+', fields: { 0: 'value-7' } }
    serialization-script: |
      out['value-7'] = inp['value-7']:as_string()
...)##";

        DYNSER_LOAD_CONFIG(ser, config::RawContents{ config });

        for (std::size_t tag_i{ 1 }; tag_i <= 7; ++tag_i) {
            const auto tag = std::to_string(tag_i);

            DYNAMIC_SECTION("Tag: " << tag)
            {
                using List = PropertyValue::ListType<PropertyValue>;

                const auto props_for_9_tag = util::map_to_props(
                    "value-7", List{ PropertyValue{ "1, " }, PropertyValue{ "2, " }, PropertyValue{ "3, " } }
                );
                const auto serialize_result{ ser.serialize_props(props_for_9_tag, tag) };

                INFO(
                    "Serialize result is: "
                    << (serialize_result ? *serialize_result
                                         : Printer{}.serialize_err_to_string(serialize_result.error()))
                );
                REQUIRE_FALSE(serialize_result);
                const auto error = serialize_result.error().error;
                REQUIRE(std::holds_alternative<serialize_err::ScriptVariableNotFound>(error));
            }
        }
    }

    SECTION("invalid tags")
    {
        DYNSER_LOAD_CONFIG(ser, config::RawContents{ "{ version: '', tags: [] }" });

        for (auto const tag : { "0", "7", "tag", "unnamed", "test", "foo" }) {
            DYNAMIC_SECTION("Tag: " << tag)
            {
                // try to serialize with invalid tag
                const auto serialize_result{ ser.serialize_props(dummy_props, tag) };

                INFO(
                    "Serialize result is: "
                    << (serialize_result ? *serialize_result
                                         : Printer{}.serialize_err_to_string(serialize_result.error()))
                );
                REQUIRE_FALSE(serialize_result);
                const auto error = serialize_result.error().error;
                REQUIRE(std::holds_alternative<serialize_err::ConfigTagNotFound>(error));
            }
        }
    }
}
