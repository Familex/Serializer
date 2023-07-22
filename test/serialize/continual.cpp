#include "common.hpp"
#include "printer.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#define DYNSER_TEST_SERIALIZE(target, tag, expected)                                                                   \
    do {                                                                                                               \
        using namespace dynser_test;                                                                                   \
        const auto serialized = ser.serialize(target, tag);                                                            \
        if (!serialized) {                                                                                             \
            UNSCOPED_INFO("Serialize error: " << Printer{}.serialize_err_to_string(serialized.error()));               \
        }                                                                                                              \
        REQUIRE(serialized);                                                                                           \
        REQUIRE(*serialized == expected);                                                                              \
    } while (false);

TEST_CASE("'Foo' rule", "[continual] [existing] [linear] [dyn-groups]")
{
    using namespace dynser_test;

    REQUIRE(trigger_load_dynser_config.load_result);

    const std::pair<Foo, std::string> poss[]{
        { { { false }, 1, 2 }, "right.1.02" },
        { { { true }, -1, -3 }, "left.-1.-03" },
        { { { false }, 0, 42 }, "right.0.42" },
        { { { true }, -2, 32 }, "left.-2.32" },
    };

    ser.context["val-length"] = dynser::PropertyValue{ "2" };
    for (const auto [pos, expected] : poss) {
        DYNSER_TEST_SERIALIZE(pos, "foo", expected);
    }
    ser.context.erase("val-length");
}

TEST_CASE("'Pos' rule", "[continual] [linear]")
{
    using namespace dynser_test;

    REQUIRE(trigger_load_dynser_config.load_result);

    const std::pair<Pos, std::string> poss[]{
        { { 1, 2 }, "1, 2" },
        { { -1, -3 }, "-1, -3" },
        { { 0, 4293291 }, "0, 4293291" },
        { { -2, 328238 }, "-2, 328238" },
    };

    for (const auto [pos, expected] : poss) {
        DYNSER_TEST_SERIALIZE(pos, "pos", expected);
    }
}

TEST_CASE("'Input' rule", "[continual] [linear] [existing]")
{
    using namespace dynser_test;

    REQUIRE(trigger_load_dynser_config.load_result);

    const std::pair<Input, std::string> inputs[]    //
        { { {}, "from: (0, 0) to: (0, 0)" },        //
          { { { -1, -1 }, { 2, 2 } }, "from: (-1, -1) to: (2, 2)" } };

    for (const auto [input, expected] : inputs) {
        DYNSER_TEST_SERIALIZE(input, "input", expected);
    }
}
