# Root CMake file
cmake_minimum_required(VERSION 3.4)
project(FX)

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

# Unicode defines
add_definitions(-DUNICODE -D_UNICODE)

add_subdirectory(third_party)
add_subdirectory(Shared)
add_subdirectory(Graphics)
add_subdirectory(Main)

if(NOT WIN32)
	# Linux Test Project
	add_subdirectory(Test)
endif(NOT WIN32)

if(WIN32)
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
endif(WIN32)