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
target_link_libraries(Main Shared)
target_link_libraries(Main Graphics)
if(WIN32)
target_link_libraries(Main zlib)
else()
target_link_libraries(Main z)
endif()
target_link_libraries(Main png)
target_link_libraries(Main vorbis)
target_link_libraries(Main ogg)
if(WIN32)
target_link_libraries(Main LZMA)
else()
target_link_libraries(Main lzma)
endif()
target_link_libraries(Main freetype)
target_link_libraries(Main SDL2)
if(NOT WIN32)
target_link_libraries(Main pthread)
target_link_libraries(Main dl)
target_link_libraries(Main GLEW)
target_link_libraries(Main GL)
endif()
target_include_directories(Main PUBLIC ${SDL2_SOURCE_DIR}/include/)