# vorbis Library
cmake_minimum_required(VERSION 3.4)

include_directories(../libogg-1.3.2/include)
add_library(vorbis lib/analysis.c
    lib/barkmel.c
    lib/bitrate.c
    lib/block.c
    lib/codebook.c
    lib/envelope.c
    lib/floor0.c
    lib/floor1.c
    lib/info.c
    lib/lookup.c
    lib/lpc.c
    lib/lsp.c
    lib/mapping0.c
    lib/mdct.c
    lib/psy.c
    lib/registry.c
    lib/res0.c
    lib/sharedbook.c
    lib/smallft.c
    lib/synthesis.c
    lib/vorbisenc.c
    lib/vorbisfile.c
    lib/window.c)
target_include_directories(vorbis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)