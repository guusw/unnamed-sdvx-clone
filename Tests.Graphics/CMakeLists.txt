# Main Project

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp")

# Compiler stuff
enable_cpp11()

include_directories(.)
add_executable(Tests.Graphics ${Main_src})
set_output_postfixes(Tests.Graphics)
	
# Dependencies
target_link_libraries(Tests.Graphics Shared)
target_link_libraries(Tests.Graphics Tests)
target_link_libraries(Tests.Graphics Graphics)

if(WIN32)
	# Bundled libraries
	target_link_libraries(Tests.Graphics zlib)
	target_link_libraries(Tests.Graphics freetype)
	target_link_libraries(Tests.Graphics SDL2)
	target_link_libraries(Tests.Graphics png)
	target_link_libraries(Tests.Graphics jpeg)
	target_include_directories(Tests.Graphics PUBLIC ${SDL2_SOURCE_DIR}/include/)
else()
	set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party)
	include(FindLibraries)
	target_link_libraries(Tests.Graphics ${ZLIB_LIBRARIES})
	target_link_libraries(Tests.Graphics ${FREETYPE_LIBRARIES})
	target_link_libraries(Tests.Graphics ${SDL2_LIBRARY})
	target_link_libraries(Tests.Graphics ${JPEG_LIBRARIES})
	target_link_libraries(Tests.Graphics ${PNG_LIBRARIES})
	target_link_libraries(Tests.Graphics pthread) 
	target_link_libraries(Tests.Graphics dl)
endif()
