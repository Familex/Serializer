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
        config_load_result = ser.load_config(config::RawContents{ config });    //
    };

    REQUIRE(config_load_result);

#define DYNSER_BENCHMARK_SERIALIZE_PROPS(ser_, props_, tag_, context_)                                                 \
    BENCHMARK("Tag: "##tag_)                                                                                           \
    {                                                                                                                  \
        ser.context = context_;                                                                                        \
        const auto result = ser.serialize_props(props_, tag_);                                                         \
        INFO(                                                                                                          \
            "Serialize result is: "                                                                                    \
            << (result ? *result : dynser_test::Printer{}.serialize_err_to_string(result.error()))                     \
        );                                                                                                             \
        REQUIRE(result);                                                                                               \
        return result;                                                                                                 \
    }

    DYNSER_BENCHMARK_SERIALIZE_PROPS(ser, Properties{}, "continual-without-fields", Context{});

#undef DYNSER_BANCHMARK_SERIALIZE_PROPS
}
