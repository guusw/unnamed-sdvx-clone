# Graphics library
cmake_minimum_required(VERSION 3.4)

# Find files used for project
add_definitions(-DGLEW_STATIC)
file(GLOB Graphics_src "src/*.cpp" "src/glew.c" "include/Graphics/*.hpp")

# Platform specific source files
if(WIN32)
	file(GLOB Graphics_src ${Graphics_src} "src/Windows/*.cpp")
else(WIN32)
	file(GLOB Graphics_src ${Graphics_src} "src/Linux/*.cpp")
endif(WIN32)

include_directories(include include/Graphics src
	include/Graphics/glew 
	${PROJECT_SOURCE_DIR}/Shared/include)
add_library(Graphics ${Graphics_src})
	
# C++11 support enabled for linux compilers
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(MSVC)
   set(PrecompiledHeaderSource src/stdafx.cpp)
   
   # Set precompiled header usage
   set_source_files_properties(${Graphics_src} PROPERTIES COMPILE_FLAGS "/Yu")
   # Set precompiled header
   set_source_files_properties(${PrecompiledHeaderSource} PROPERTIES COMPILE_FLAGS "/Yc")
   # Ignore glew.c from precompiled source
   set_source_files_properties("src/glew.c" PROPERTIES COMPILE_FLAGS "")
endif(MSVC)

target_link_libraries(Graphics png)
target_link_libraries(Graphics jpeg)
target_include_directories(Graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find freetype library based on platform
if(NOT WIN32)
	# Find Freetype as a linux package
	find_package(Freetype REQUIRED)
	message(STATUS "Freetype detected at: ${FREETYPE_INCLUDE_DIRS}")
	include_directories(${FREETYPE_INCLUDE_DIRS})
else(NOT WIN32)
	# Link against bundled freetype
	target_link_libraries(Graphics freetype)
endif(NOT WIN32)