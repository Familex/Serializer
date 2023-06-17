add_library (
    dynser
    "dynser/dynser.cpp"
    "dynser/dynser.h"
    "dynser.h"
    "structs/fields.hpp"
    "structs/properties.hpp"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET dynser PROPERTY CXX_STANDARD 20)
endif()

target_include_directories (
    dynser 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
