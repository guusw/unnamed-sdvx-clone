# ogg Library
cmake_minimum_required(VERSION 3.6)

add_library(ogg src/bitwise.c src/framing.c)
target_include_directories(ogg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)