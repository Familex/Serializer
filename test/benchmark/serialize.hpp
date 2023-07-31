#include "dynser.h"
#include "printer.hpp"
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Serialize")
{
    using namespace dynser;

    const auto config = R"##(---
version: ''
tags:
  - name: "continual-without-fields"
    continual:
      - linear: { pattern: 'lorem ipsum' }
      - linear: { pattern: '(?:lorem ipsum){20}' }
      - linear: { pattern: '.' }
  - name: "empty"
    continual: []
  - name: "minimal-lua"
    continual:
      - linear: { pattern: '.*', fields: { 0: value } }
    serialization-script: |
      out['value'] = inp['value']:as_string()
  - name: "recurrent-empty"
    recurrent:
      - linear: { pattern: '.*', fields: { 0: len-expander } }
    serialization-script: |
      out['len-expander'] = inp['len-expander']:as_string()
  - name: "recurrent-existing-empty"
    recurrent:
      - linear: { pattern: '.*', fields: { 0: len-expander } }
      - existing: { tag: "empty" }
    serialization-script: |
      out['len-expander'] = inp['len-expander']:as_string()
...)##";

    DynSer ser{
        //
        generate_property_to_target_mapper(),
        generate_target_to_property_mapper()
        //
    };

    config::ParseResult config_load_result;

    BENCHMARK("load config")
    {
        const auto load_result{ ser.load_config(config::RawContents{ config }) };
        config_load_result = load_result;
        return load_result;
    };

    REQUIRE(config_load_result);

#define DYNSER_BENCHMARK_SERIALIZE_PROPS(ser_, props_, tag_, context_)                                                 \
    BENCHMARK("Tag: "##tag_)                                                                                           \
    {                                                                                                                  \
        const auto context = context_;                                                                                 \
        const auto props = props_;                                                                                     \
        const auto tag = tag_;                                                                                         \
        ser_.context = context;                                                                                        \
        const auto result = ser.serialize_props(props, tag);                                                           \
        INFO(                                                                                                          \
            "Serialize result is: "                                                                                    \
            << (result ? *result : dynser_test::Printer{}.serialize_err_to_string(result.error()))                     \
        );                                                                                                             \
        REQUIRE(result);                                                                                               \
        ser_.context.clear();                                                                                          \
        return result;                                                                                                 \
    }

    using List = PropertyValue::ListType<PropertyValue>;

    DYNSER_BENCHMARK_SERIALIZE_PROPS(ser, Properties{}, "continual-without-fields", Context{});
    DYNSER_BENCHMARK_SERIALIZE_PROPS(ser, Properties{}, "empty", Context{});
    DYNSER_BENCHMARK_SERIALIZE_PROPS(ser, util::map_to_props("value", "lorem ipsum"), "minimal-lua", Context{});
    DYNSER_BENCHMARK_SERIALIZE_PROPS(
        ser, util::map_to_props("len-expander", List(100ull, PropertyValue{ "" })), "recurrent-empty", Context{}
    );
    DYNSER_BENCHMARK_SERIALIZE_PROPS(
        ser,
        util::map_to_props("len-expander", List(100ull, PropertyValue{ "" })),
        "recurrent-existing-empty",
        Context{}
    );

#undef DYNSER_BANCHMARK_SERIALIZE_PROPS
}
