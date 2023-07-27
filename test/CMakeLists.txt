cmake_minimum_required (VERSION 3.8)

include (FetchContent)

FetchContent_Declare (
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable (Catch2)

list (APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include (CTest)
include (Catch)

add_executable (
    internal-tests

    internal/dyn_regex.hpp
    internal/regex_parse.hpp
    internal/regex_to_string.hpp

    internal/tests.cpp
)

add_executable (
    serialize-tests

    serialize/common.hpp
    serialize/printer.hpp

    serialize/continual.hpp
    serialize/branched.hpp
    serialize/recursive.hpp
    serialize/recurrent.hpp
    serialize/error_cases.hpp

    serialize/tests.cpp
)

add_executable (
    catch-test

    catch/catch_test.cpp
)

set ("tests" "internal-tests" "serialize-tests" "catch-test")
set ("dynser-tests" "internal-tests" "serialize-tests")

foreach (targ ${dynser-tests})
    target_link_libraries (
        ${targ}
        PRIVATE
        dynser
    )
endforeach ()

foreach (targ ${tests})
    # compile warnings
    target_compile_options (
        ${targ}
        INTERFACE
        "$<$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
        "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:$<BUILD_INTERFACE:-W3>>"
    )

    # cpp version
    set_property (TARGET ${targ} PROPERTY CXX_STANDARD 23)

    target_link_libraries (${targ} PRIVATE Catch2::Catch2WithMain)

    catch_discover_tests (${targ})
endforeach ()
