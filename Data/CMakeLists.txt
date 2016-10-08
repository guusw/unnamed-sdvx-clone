# Data/Text Serialization Library

# Find files used for project
file(GLOB src "src/*.cpp")
file(GLOB include "include/Data/*.hpp")
file(GLOB libYAML_src "src/libYAML/src/*.c")
source_group("Source Files" FILES ${src})
source_group("Source Files\\libYAML" FILES ${libYAML_src})
source_group("Header Files" FILES ${include})

# Compiler stuff
enable_cpp11()
enable_precompiled_headers("${src}" src/stdafx.cpp)
precompiled_header_exclude("${libYAML_src}")

add_definitions(-DYAML_DECLARE_STATIC -DHAVE_CONFIG_H)

include_directories(include include/Data src src/libYAML/src src/libYAML/include)
add_library(Data ${src} ${include} ${libYAML_src})

# Public include paths for library
target_include_directories(Data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Dependencies
target_link_libraries(Data Shared)