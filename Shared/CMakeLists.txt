# Shared Library

# Find files used for project
file(GLOB Shared_src "src/*.cpp" "include/Shared/*.hpp")

# Platform specific source files
if(WIN32)
	file(GLOB Shared_src ${Shared_src} "src/Windows/*.cpp")
else(WIN32)
	file(GLOB Shared_src ${Shared_src} "src/Linux/*.cpp")
endif(WIN32)

include_directories(include include/Shared src)
add_library(Shared ${Shared_src})

# Public include paths for library
target_include_directories(Shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Compiler stuff
enable_cpp11()
enable_precompiled_headers(${Shared_src} src/stdafx.cpp)