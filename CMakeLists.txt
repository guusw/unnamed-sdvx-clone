# Root CMake file
cmake_minimum_required(VERSION 3.4)
project(FX)

# Additional modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Project configurations
set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_DEBUG_POSTFIX _Debug)
set(CMAKE_RELEASE_POSTFIX _Release)

# Set output folders
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/bin )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/bin )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${PROJECT_SOURCE_DIR}/lib )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# All projects use unicode define
#	this is mainly for windows functions either being defined to call A or W prefixed functions
add_definitions(-DUNICODE -D_UNICODE)

# Begin defining functions/macros for use in sub-projects

# Precompiled header macro
#	src 	= Path to source files
#	pchSrc 	= Path to precompiled header source file
macro(enable_precompiled_headers src pchSrc)
	if(MSVC)
	   # Set precompiled header usage
	   set_source_files_properties(src PROPERTIES COMPILE_FLAGS "/Yu")
	   # Set precompiled header
	   set_source_files_properties(pchSrc PROPERTIES COMPILE_FLAGS "/Yc")
	endif(MSVC)
endmacro(enable_precompiled_headers)

# Excludes a file from precompiled header usage
macro(precompiled_header_exclude exclude)
	if(MSVC)
		# Excluded files
	   set_source_files_properties(exclude PROPERTIES COMPILE_FLAGS "")
	endif(MSVC)
endmacro(precompiled_header_exclude)

# Function to enable c++11 compilation on linux
macro(enable_cpp11)
	if(UNIX)
		# C++11 support enabled for linux compilers
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	endif(UNIX)
endmacro(enable_cpp11)

# Set output binary postfixes so that they will be named <project>_<configuration>.exe/dll
macro(set_output_postfixes projectName)
	set_target_properties(${projectName} PROPERTIES 
		OUTPUT_NAME_DEBUG ${projectName}_Debug
		OUTPUT_NAME_RELEASE ${projectName}_Release)
endmacro(set_output_postfixes)

# Sub-Project directories
add_subdirectory(third_party)
add_subdirectory(Shared)
add_subdirectory(Graphics)
add_subdirectory(Main)

# Linux Test Project
#if(UNIX)
#	add_subdirectory(Test)
#endif(UNIX)

# Enabled project filters on windows
if(MSVC)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	
	# Then in some target add this to put the target in the Development folder.
	set_target_properties(jpeg PROPERTIES FOLDER "Third Party")
	set_target_properties(png PROPERTIES FOLDER "Third Party")
	set_target_properties(zlib PROPERTIES FOLDER "Third Party")
	set_target_properties(LZMA PROPERTIES FOLDER "Third Party")
	set_target_properties(ogg PROPERTIES FOLDER "Third Party")
	set_target_properties(vorbis PROPERTIES FOLDER "Third Party")
	set_target_properties(freetype PROPERTIES FOLDER "Third Party")
	set_target_properties(Shared PROPERTIES FOLDER Libraries)
	set_target_properties(Graphics PROPERTIES FOLDER Libraries)
endif(MSVC)