

IF(MSVC)
SET(MINIZIP_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/adler32.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/compress.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/crc32.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/crc32.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/crypt.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/deflate.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/deflate.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzclose.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzguts.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzlib.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzread.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzwrite.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/infback.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inffast.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inffast.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inffixed.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inflate.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inflate.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inftrees.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inftrees.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/ioapi.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/ioapi.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/iowin32.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/iowin32.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/miniunz.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/mztools.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/mztools.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/trees.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/trees.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/uncompr.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/unzip.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/unzip.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zconf.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zip.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zip.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zlib.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zutil.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zutil.h
        PARENT_SCOPE
)
ELSE()
SET(MINIZIP_SRC
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/adler32.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/compress.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/crc32.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/crc32.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/crypt.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/deflate.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/deflate.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzclose.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzguts.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzlib.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzread.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/gzwrite.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/infback.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inffast.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inffast.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inffixed.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inflate.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inflate.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inftrees.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/inftrees.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/ioapi.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/ioapi.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/miniunz.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/mztools.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/mztools.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/trees.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/trees.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/uncompr.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/unzip.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/unzip.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zconf.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zip.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zip.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zlib.h
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zutil.c
		${CMAKE_CURRENT_SOURCE_DIR}/minizip/zutil.h
        PARENT_SCOPE
)
ENDIF()


# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/blosc)
SET(BLOSC_SRC ${BLOSC_SRC} PARENT_SCOPE)

SET(JPEG9B_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jpeg-9b PARENT_SCOPE)
SET(CHARLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/charls PARENT_SCOPE)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/jpeg-9b JPEG9B_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/charls CHARLS_SRC)

SET(JPEG9B_SRC ${JPEG9B_SRC} PARENT_SCOPE)
SET(CHARLS_SRC ${CHARLS_SRC} PARENT_SCOPE)


SET(EXTRAS_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
