add_library (Serializer_lib "Serializer.cpp" "Serializer.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Serializer_lib PROPERTY CXX_STANDARD 20)
endif()
