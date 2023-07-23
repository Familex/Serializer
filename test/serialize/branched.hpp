#include "common.hpp"

TEST_CASE("Branched rule")
{
    using namespace dynser_test;

    REQUIRE(trigger_load_dynser_config.load_result);

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
