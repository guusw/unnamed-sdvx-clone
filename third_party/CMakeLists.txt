# All third party libraries
cmake_minimum_required(VERSION 3.4)

if(WIN32)
	add_subdirectory(freetype)
	add_subdirectory(jpeg-9b)
	add_subdirectory(LZMA)
	add_subdirectory(zlib-1.2.8)
	add_subdirectory(SDL2-2.0.4)
endif(WIN32)
add_subdirectory(lpng1621)
add_subdirectory(libogg-1.3.2)
add_subdirectory(libvorbis-1.3.5)