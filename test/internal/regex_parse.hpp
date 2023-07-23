#include "dynser.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Regex parse check")
{
    using namespace dynser::config::details::regex;

    struct Test
    {
        std::expected<Regex, std::size_t> test;
        std::optional<std::size_t> error{ std::nullopt };
    };

    const Test tests[]{
        { from_string("(a)(b)((c)((d)))") },
        { from_string("((ke(((\\.+)+){0,0})?){20,25}ab)") },
        { from_string("(?=[lookahead])") },
        { from_string("(?![negative]|[lookahead])") },
        { from_string("(?<=[lookbehind]*)") },
        { from_string("(?<![negative][ ][lookbehind]?)") },
        { from_string("(\\d+)\\.") },
        { from_string("(\\w*).{5,256}?") },
        { from_string("a|b") },
        { from_string("[ab]+|b") },
        { from_string("|a|b|3") },
        { from_string("[astf]{2,}|futnun[^tfnu]*") },
        { from_string("[astf]{2,}|futnun[^tfnu]*") },
        { from_string("[\\d\\S\\]]{2,}\\10+") },
        { from_string("(\\d{2,4})-(\\d{2,4})-(\\d{2,4})") },
        { from_string(" [ "), 3 },
        { from_string("["), 1 },
        { from_string("("), 1 },
        { from_string(")"), 0 },
        { from_string("]"), 0 },
    };

    for (std::size_t num{}; const auto& test : tests) {
        DYNAMIC_SECTION("Regex# " << num)
        {
            if (!test.test && !test.error) {
                INFO("error on " << test.test.error() << " index");
                CHECK(false);
            }
            else if (!test.test && test.test.error() != test.error) {
                INFO("expected error " << *test.error << " not equal to actual error: " << test.test.error());
                CHECK(false);
            }
            else {
                CHECK(true);
            }
        }
        ++num;
    }
}
