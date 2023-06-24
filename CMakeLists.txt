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
    "src/util/collection_print.hpp"
    "src/util/lua_manual_debug.hpp"
)

set_property (TARGET main PROPERTY CXX_STANDARD 23)

# yaml configs
file(
    COPY "${CMAKE_CURRENT_SOURCE_DIR}/yaml"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)

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
)

