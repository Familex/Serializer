#include "common.hpp"

TEST_CASE("Continual rule")
{
    using namespace dynser_test;

    REQUIRE(trigger_load_dynser_config.load_result);

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
