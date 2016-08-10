# Main Project

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp"  "Resource.rc" 
	"Audio/*.cpp" "Audio/*.hpp"
	"GUI/*.cpp" "GUI/*.hpp")
file(GLOB C_src "*.c")

# Project filters
file(GLOB Audio_hdr "Audio/*.hpp")
file(GLOB Audio_src "Audio/*.cpp")
source_group("Header Files\\Audio" FILES ${Audio_hdr})
source_group("Source Files\\Audio" FILES ${Audio_src})
file(GLOB GUI_hdr "GUI/*.hpp")
file(GLOB GUI_src "GUI/*.cpp")
source_group("Header Files\\GUI" FILES ${GUI_hdr})
source_group("Source Files\\GUI" FILES ${GUI_src})

include_directories(Audio GUI .)
add_executable(Main ${Main_src} ${C_src})
set_output_postfixes(Main)

# Compiler stuff
enable_cpp11()
enable_precompiled_headers(${Main_src} src/stdafx.cpp)	
precompiled_header_exclude(${C_src})

if(NOT WIN32)
	add_definitions(-DAUDIO_SDL)
endif(NOT WIN32)

# Target subsystem on windows
if(MSVC)
   set_target_properties(Main PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif(MSVC)
	
# Dependencies
if(UNIX)
	set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party)
	include(FindPackages)
endif(UNIX)
target_link_libraries(Main Shared)
target_link_libraries(Main Graphics)
if(WIN32)
	target_link_libraries(Main zlib)
	target_link_libraries(Main LZMA)
	target_link_libraries(Main freetype)
	target_link_libraries(Main SDL2)
else()
	target_link_libraries(Main ${ZLIB_LIBRARIES})
	target_link_libraries(Main ${LIBLZMA_LIBRARIES})
	target_link_libraries(Main ${FREETYPE_LIBRARIES})
	target_link_libraries(Main ${SDL2_LIBRARY})
	target_link_libraries(Main pthread) # <-
	target_link_libraries(Main dl)      #   | TODO
	target_link_libraries(Main GLEW)    #   |  find_package()
	target_link_libraries(Main GL)      # <-
endif()
target_link_libraries(Main png)
target_link_libraries(Main vorbis)
target_link_libraries(Main ogg)
target_include_directories(Main PUBLIC ${SDL2_SOURCE_DIR}/include/)