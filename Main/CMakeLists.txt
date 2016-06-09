# Main Project
cmake_minimum_required(VERSION 3.4)

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp" "Audio/*.cpp" "Audio/*.hpp" "Resource.rc")

include_directories(Audio .)
add_executable(Main ${Main_src})
set_target_properties(Main PROPERTIES 
	OUTPUT_NAME_DEBUG Main_Debug
	OUTPUT_NAME_RELEASE Main_Release)
	
if(MSVC)
   set_target_properties(Main PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
   
   set(PrecompiledHeaderSource stdafx.cpp)
   
   # Set precompiled header usage
   set_source_files_properties(${Main_src} PROPERTIES COMPILE_FLAGS "/Yu")
   # Set precompiled header
   set_source_files_properties(${PrecompiledHeaderSource} PROPERTIES COMPILE_FLAGS "/Yc")
endif(MSVC)
	
target_link_libraries(Main Shared)
target_link_libraries(Main Graphics)
target_link_libraries(Main zlib)
target_link_libraries(Main png)
target_link_libraries(Main ogg)
target_link_libraries(Main vorbis)
target_link_libraries(Main LZMA)
target_link_libraries(Main freetype)