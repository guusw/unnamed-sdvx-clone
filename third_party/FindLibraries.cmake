# Find linux packages
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(SDL2 REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Vorbis REQUIRED)
find_package(OGG REQUIRED)

# Linux include directories
include_directories(
	${FREETYPE_INCLUDE_DIRS}
	${ZLIB_INCLUDE_DIRS}
	${SDL2_INCLUDE_DIR}
	${VORBIS_INCLUDE_DIR}
	${OGG_INCLUDE_DIR})
