add_library (serializer "serializer/serializer.cpp" "serializer/serializer.h" "serializer.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET serializer PROPERTY CXX_STANDARD 20)
endif()

target_include_directories (
    serializer 
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
