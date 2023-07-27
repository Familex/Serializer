#include "common.hpp"

TEST_CASE("Error cases")
{
    using namespace dynser;
    using namespace dynser_test;

    const auto dummy_props{ dynser::Properties{} };

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

        auto ser = get_dynser_instance();

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
}
