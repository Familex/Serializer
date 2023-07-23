#include "common.hpp"

TEST_CASE("Recursive rule")
{
    using namespace dynser_test;

    REQUIRE(trigger_load_dynser_config.load_result);

    SECTION("'recursive' rule", "[continual] [recursive] [existing] [linear] [required]")
    {
        std::pair<std::vector<int>, std::string> recursives[]{
            //
            { { 1, 2, 3 }, "[ 1, 2, 3 ]" },
            // { {}, "[  ]" }, // not implemented
            { { -1, -3, 10, 0, 9999 }, "[ -1, -3, 10, 0, 9999 ]" },
            //
        };

        for (auto const& [recursive, expected] : recursives) {
            DYNSER_TEST_SERIALIZE(recursive, "recursive", expected);
        }
    }
}
