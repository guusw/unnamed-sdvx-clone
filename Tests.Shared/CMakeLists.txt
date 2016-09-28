# Main Project

# Find files used for project
file(GLOB Main_src "*.cpp" "*.hpp")

# Compiler stuff
enable_cpp11()

include_directories(.)
add_executable(Tests.Shared ${Main_src})
set_output_postfixes(Tests.Shared)
enable_precompiled_headers("${Main_src}" stdafx.cpp)

if(MSVC)
	# SEH and Debug
	target_compile_options(Tests.Shared PUBLIC /EHa /Zi)
else()
	target_compile_options(Tests.Shared PUBLIC -rdynamic)
endif()

# Dependencies
target_link_libraries(Tests.Shared Shared)
target_link_libraries(Tests.Shared Tests)
target_link_libraries(Tests.Shared Data)

if(WIN32)
else()
	set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/third_party)
	include(FindLibraries)
endif()
