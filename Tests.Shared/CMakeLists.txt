# Main Project

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp")

# Compiler stuff
enable_cpp11()

include_directories(.)
add_executable(Tests.Shared ${Main_src})
set_output_postfixes(Tests.Shared)
	
# Dependencies
target_link_libraries(Tests.Shared Shared)

if(WIN32)
	# Bundled libraries
	target_link_libraries(Tests.Shared zlib)
else()
	set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party)
	include(FindLibraries)
	target_link_libraries(Tests.Shared ${ZLIB_LIBRARIES})
	target_link_libraries(Tests.Shared pthread) 
	target_link_libraries(Tests.Shared dl)
endif()
