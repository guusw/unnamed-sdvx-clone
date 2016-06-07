# Graphics library
cmake_minimum_required(VERSION 3.4)

# Find files used for project
add_definitions(-DGLEW_STATIC)
file(GLOB Graphics_src "src/*.cpp" "src/glew.c")

include_directories(include 
	include/Graphics 
	include/Graphics/glew 
	${PROJECT_SOURCE_DIR}/Shared/include)
	
	
# C++11 support enabled for linux compilers
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	
add_library(Graphics ${Graphics_src})
target_link_libraries(Graphics png)
target_link_libraries(Graphics jpeg)
target_include_directories(Graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find libraries on linux
if(NOT WIN32)
	find_package(Freetype REQUIRED)
	message(STATUS "Freetype detected at: ${FREETYPE_INCLUDE_DIRS}")
	include_directories(${FREETYPE_INCLUDE_DIRS})
endif(NOT WIN32)