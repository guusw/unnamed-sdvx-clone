# PNG Library
cmake_minimum_required(VERSION 3.4)

include_directories(../zlib-1.2.8)
add_library(png png.c
    pngerror.c
    pngget.c
    pngmem.c
    pngpread.c
    pngread.c
    pngrio.c
    pngrtran.c
    pngrutil.c
    pngset.c
    pngtrans.c
    pngwio.c
    pngwrite.c
    pngwtran.c
    pngwutil.c)
target_include_directories(png PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})