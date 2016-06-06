cmake_minimum_required(VERSION 3.6)

# Find files used for project
file(GLOB Shared_src "src/*.cpp" "include/Shared/*.hpp")

include_directories(include include/Shared)
add_library(Shared ${Shared_src})

if(WIN32)
   set(PrecompiledHeaderSource src/stdafx.cpp)
   
   # Set precompiled header usage
   set_source_files_properties(${Shared_src} PROPERTIES COMPILE_FLAGS "/Yu")
   # Set precompiled header
   set_source_files_properties(${PrecompiledHeaderSource} PROPERTIES COMPILE_FLAGS "/Yc")
   
endif(WIN32)

target_include_directories(Shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)