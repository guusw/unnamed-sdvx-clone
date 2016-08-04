# Graphics library

# Find files used for project
file(GLOB Graphics_src "src/*.cpp" "src/glew.c" "include/Graphics/*.hpp")

# Platform specific source files
if(WIN32)
	include_directories("src/Windows")
	file(GLOB Platform_src "src/Windows/*.cpp" "src/Windows/*.hpp")
	# Platform specific source group
	source_group("Source Files\\Windows" FILES ${Platform_src})
else(WIN32)
#include_directories("src/Linux")
	include_directories("src/Windows")
#	file(GLOB Platform_src "src/Linux/*.cpp")
	file(GLOB Platform_src "src/Windows/*.cpp")
endif(WIN32)

file(GLOB Graphics_src ${Graphics_src} ${Platform_src})

include_directories(include include/Graphics src
	include/Graphics/glew 
	${PROJECT_SOURCE_DIR}/Shared/include)
add_library(Graphics ${Graphics_src})

# Public include paths for library
target_include_directories(Graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
	
# GLEW is included statically and also doesn't need GLU(Which doesn't even exist on linux)
add_definitions(-DGLEW_STATIC -DGLEW_NO_GLU)
	
# Compiler stuff
enable_cpp11()
enable_precompiled_headers(${Graphics_src} src/stdafx.cpp)
precompiled_header_exclude(src/glew.c)

# Dependencies
target_link_libraries(Graphics png)
target_link_libraries(Graphics jpeg)
target_link_libraries(Graphics Shared)
target_link_libraries(Graphics SDL2)
target_include_directories(Graphics PUBLIC ${SDL2_SOURCE_DIR}/include/)
if(UNIX)
	# Find Freetype as a linux package
	find_package(Freetype REQUIRED)
	include_directories(${FREETYPE_INCLUDE_DIRS})
else(UNIX)
	# Link against bundled freetype
	target_link_libraries(Graphics freetype)
endif(UNIX)