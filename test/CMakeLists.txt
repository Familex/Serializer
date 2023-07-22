cmake_minimum_required (VERSION 3.8)

include (FetchContent)

FetchContent_Declare (
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable (Catch2)

add_executable (
    serialize-tests

    serialize/common.hpp
    serialize/printer.hpp

    serialize/continual.cpp
    serialize/branched.cpp
    serialize/recursive.cpp
    serialize/recurrent.cpp
)

add_executable (
    catch-test

    catch/catch_test.cpp
)

# compile warnings
target_compile_options (
    serialize-tests
    INTERFACE
    "$<$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:$<BUILD_INTERFACE:-W3>>"
)

set_property (TARGET serialize-tests PROPERTY CXX_STANDARD 23)

target_link_libraries (
    serialize-tests
    PRIVATE
    dynser
)

target_link_libraries (serialize-tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries (catch-test PRIVATE Catch2::Catch2WithMain)

list (APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include (CTest)
include (Catch)

catch_discover_tests (serialize-tests)
catch_discover_tests (catch-test)

