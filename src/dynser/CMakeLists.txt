#######################################
# Source
#######################################

add_library (
    dynser
    "dynser.h"

    "dynser/dynser.h" "dynser/dynser.cpp"
    "structs/fields.hpp"
    "structs/properties.hpp"
    "structs/context.hpp"
    "util/prefix.hpp"
    "config/config.h" "config/config.cpp"
    "config/regex/structures.h" "config/regex/structures.cpp"
    "config/regex/to_string.h" "config/regex/to_string.cpp"
    "config/regex/from_string.h" "config/regex/from_string.cpp"
    "config/structures.h"
)

set_property (TARGET dynser PROPERTY CXX_STANDARD 23)

include_directories (
    dynser
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

#######################################
# External projects
#######################################

include (../download_project/DownloadProject.cmake)

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

#######################################
# Link
#######################################

target_link_libraries (
    dynser
    lua_static
)

#######################################
# Paths
#######################################

target_include_directories (
    dynser 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${luwra_SOURCE_DIR}/lib"
)
