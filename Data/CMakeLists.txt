# Data/Text Serialization Library

# Find files used for project
file(GLOB src "src/*.cpp")
file(GLOB include "include/Data/*.hpp")
source_group("Source Files" FILES ${src})
source_group("Header Files" FILES ${include})

# Compiler stuff
enable_cpp11()
enable_precompiled_headers("${src}" src/stdafx.cpp)

include_directories(include include/Data src)
add_library(Data ${src} ${include})

# Public include paths for library
target_include_directories(Data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Dependencies
target_link_libraries(Data Shared)