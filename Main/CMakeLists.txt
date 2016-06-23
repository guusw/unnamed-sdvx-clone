# Main Project

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp" "Audio/*.cpp" "Audio/*.hpp" "Resource.rc")

include_directories(Audio .)
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