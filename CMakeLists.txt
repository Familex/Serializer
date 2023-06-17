cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy (SET CMP0141 NEW)
  set (CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif ()

project (main)

#######################################
# External projects
#######################################

include (src/download_project/DownloadProject.cmake)

# lua_static
download_project (
    PROJ lua_cmake
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG master
    UPDATE_DISCONNECTED 1
)
add_subdirectory (${lua_cmake_SOURCE_DIR} ${lua_cmake_BINARY_DIR})
include_directories ("${lua_cmake_SOURCE_DIR}/lua-5.4.5/src")

# luwra (header-only)
download_project (
    PROJ luwra
    GIT_REPOSITORY https://github.com/vapourismo/luwra.git
    GIT_TAG master
    UPDATE_DISCONNECTED 1
)
include_directories ("${luwra_SOURCE_DIR}/lib")

# yaml-cpp
download_project (
    PROJ yaml_cpp 
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG master
    UPDATE_DISCONNECTED 1
)
add_subdirectory (${yaml_cpp_SOURCE_DIR} ${yaml_cpp_BINARY_DIR})
include_directories ("${yaml_cpp_SOURCE_DIR}/include")

#######################################
# Source
#######################################

add_subdirectory ("src/dynser")

add_executable (
    main
    "src/main.cpp"
    "src/dynser/utils/collection_print.hpp"
    "src/dynser/utils/lua_manual_debug.hpp"
)

set_property (TARGET main PROPERTY CXX_STANDARD 23)

# compile warnings
target_compile_options (
    main
    INTERFACE
    "$<$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:$<BUILD_INTERFACE:-W3>>"
)

#######################################
# Link
#######################################

# link
target_link_libraries (
    main
    dynser
    # downloaded libs
    yaml-cpp
    lua_static
)

