# LZMA Library
cmake_minimum_required(VERSION 3.4)

add_library(LZMA 7zAlloc.c
    7zArcIn.c
    7zBuf.c
    7zBuf2.c
    7zCrc.c
    7zCrcOpt.c
    7zDec.c
    7zFile.c
    7zStream.c
    Aes.c
    AesOpt.c
    Alloc.c
    Bcj2.c
    Bcj2Enc.c
    Bra.c
    Bra86.c
    BraIA64.c
    CpuArch.c
    Delta.c
    LzFind.c
    LzFindMt.c
    Lzma2Dec.c
    Lzma2Enc.c
    Lzma86Dec.c
    Lzma86Enc.c
    LzmaDec.c
    LzmaEnc.c
    LzmaLib.c
    MtCoder.c
    Ppmd7.c
    Ppmd7Dec.c
    Ppmd7Enc.c
    Sha256.c
    Sort.c
    Source.cpp
    Threads.c
    Xz.c
    XzCrc64.c
    XzCrc64Opt.c
    XzDec.c
    XzEnc.c
    XzIn.c)
target_include_directories(LZMA PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})