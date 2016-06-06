# All third party libraries
cmake_minimum_required(VERSION 3.6)

if(WIN32)
	add_subdirectory(freetype)
	add_subdirectory(jpeg-9b)
	add_subdirectory(libogg-1.3.2)
	add_subdirectory(libvorbis-1.3.5)
	add_subdirectory(lpng1621)
	add_subdirectory(LZMA)
	add_subdirectory(zlib-1.2.8)
endif(WIN32)
	