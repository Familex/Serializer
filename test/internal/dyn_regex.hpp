#include "dynser.h"
#include <catch2/catch_test_macros.hpp>

#include <regex>

template <typename T>
inline T clone(T const& val) noexcept
{
    return val;
}

TEST_CASE("Dyn regex")
{
    using namespace dynser::config::details;

    const auto dyn_regex = yaml::DynRegex{ R"(\_2\[\d{\_1},\d{\_1}\])" };
    const auto without_prefix = resolve_dyn_regex(clone(dyn_regex), yaml::DynGroupValues{ { 1, "4" } });

    CHECK(std::regex_match("[0010,1999]", std::regex{ without_prefix }));

    const auto with_prefix = resolve_dyn_regex(clone(dyn_regex), yaml::DynGroupValues{ { 1, "2" }, { 2, "pos: " } });

    CHECK(std::regex_match("pos: [04,12]", std::regex{ with_prefix }));
    CHECK(!std::regex_match("pos:[04,12]", std::regex{ with_prefix }));
    CHECK(!std::regex_match("pos: [4,12]", std::regex{ with_prefix }));
}
