# Main Project

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp"  "Resource.rc" 
	"Audio/*.cpp" "Audio/*.hpp"
	"GUI/*.cpp" "GUI/*.hpp")

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
add_executable(Main ${Main_src})
set_output_postfixes(Main)

# Compiler stuff
enable_cpp11()
enable_precompiled_headers(${Main_src} src/stdafx.cpp)	

# Target subsystem on windows
if(MSVC)
   set_target_properties(Main PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif(MSVC)
	
# Dependencies
target_link_libraries(Main Shared)
target_link_libraries(Main Graphics)
target_link_libraries(Main zlib)
target_link_libraries(Main png)
target_link_libraries(Main ogg)
target_link_libraries(Main vorbis)
target_link_libraries(Main LZMA)
target_link_libraries(Main freetype)