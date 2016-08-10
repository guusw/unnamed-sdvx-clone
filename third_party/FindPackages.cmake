find_package(Freetype REQUIRED)
find_package(LibLZMA REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SDL2 REQUIRED)

# Linux setup
include_directories (
	${FREETYPE_INCLUDE_DIRS}
	${LIBLZMA_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIR}
)
