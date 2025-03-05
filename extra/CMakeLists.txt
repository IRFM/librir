include(ExternalProject)
include(FetchContent)

set(3RD_64_INSTALL_DIR ${CMAKE_BINARY_DIR}/3rd-party-install CACHE INTERNAL "")

if(BUILD_OFFLINE)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY EP_UPDATE_DISCONNECTED 1)
else()
    # set(EP_DOWNLOAD_COMMAND DOWNLOAD_COMMAND 1)
endif()

cmake_policy(SET CMP0114 NEW)
cmake_policy(SET CMP0135 NEW)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30.0")
    cmake_policy(SET CMP0169 OLD)
endif()

set(ZSTD_INSTALL_DIR ${CMAKE_BINARY_DIR}/3rd-party-install)
set(ZSTD_MINIMUM_VERSION 1.5.5)

# we are statically building from ZSTD
set(ZSTD_BUILD_SHARED OFF CACHE INTERNAL "" FORCE)
FetchContent_Declare(
    zstd
    GIT_REPOSITORY "https://github.com/facebook/zstd.git"
    GIT_TAG v${ZSTD_MINIMUM_VERSION}
    OVERRIDE_FIND_PACKAGE
    SOURCE_SUBDIR build/cmake
)
FetchContent_GetProperties(zstd)

if(NOT zstd_POPULATED)
    FetchContent_Populate(zstd)
    add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(ZSTD_INCLUDE_DIR ${zstd_SOURCE_DIR}/lib CACHE INTERNAL "")

# minizip
FetchContent_Declare(
    minizip
    GIT_REPOSITORY "https://github.com/zlib-ng/minizip-ng.git"
    GIT_TAG 4.0.7
    EXCLUDE_FROM_ALL
    OVERRIDE_FIND_PACKAGE
)
message(STATUS "Fetching minizip")
set(MZ_ZLIB OFF CACHE INTERNAL "" FORCE)
set(MZ_BZIP2 OFF CACHE INTERNAL "" FORCE)
set(MZ_LZMA OFF CACHE INTERNAL "" FORCE)
set(MZ_ZSTD OFF CACHE INTERNAL "" FORCE)
set(MZ_PKCRYPT OFF CACHE INTERNAL "" FORCE)
set(MZ_WZAES OFF CACHE INTERNAL "" FORCE)
set(MZ_ICONV OFF CACHE INTERNAL "" FORCE)
set(MZ_OPENSSL OFF CACHE INTERNAL "" FORCE)
set(MZ_LIBBSD OFF CACHE INTERNAL "" FORCE)
set(ZSTD_DIR ${zstd_DIR} CACHE INTERNAL "" FORCE)

FetchContent_GetProperties(minizip)

if(NOT minizip_POPULATED)
    FetchContent_Populate(minizip)

    # we need to add the subdirectory to get unzip.h compat interface
    add_subdirectory(${minizip_SOURCE_DIR} ${minizip_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# set(ZSTD_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/zstd-src/lib CACHE INTERNAL "" FORCE)
set(MINIZIP_SOURCES
    ${minizip_SOURCE_DIR}/mz_crypt.c
    ${minizip_SOURCE_DIR}/mz_os.c
    ${minizip_SOURCE_DIR}/mz_strm.c
    ${minizip_SOURCE_DIR}/mz_strm_buf.c
    ${minizip_SOURCE_DIR}/mz_strm_mem.c
    ${minizip_SOURCE_DIR}/mz_strm_split.c
    ${minizip_SOURCE_DIR}/mz_zip.c
    ${minizip_SOURCE_DIR}/mz_strm_zstd.c

    # ${minizip_SOURCE_DIR}/mz_zip_rw.c
    ${minizip_SOURCE_DIR}/mz_compat.c

    # ZSTD
    ${minizip_SOURCE_DIR}/mz_strm_zstd.c
)

if(MSVC OR WIN32)
    list(APPEND MINIZIP_SOURCES ${minizip_SOURCE_DIR}/mz_os_win32.c)
    list(APPEND MINIZIP_SOURCES ${minizip_SOURCE_DIR}/mz_strm_os_win32.c)
elseif(LINUX)
    list(APPEND MINIZIP_SOURCES ${minizip_SOURCE_DIR}/mz_os_posix.c)
    list(APPEND MINIZIP_SOURCES ${minizip_SOURCE_DIR}/mz_strm_os_posix.c)
endif()

set(MINIZIP_HEADERS
    ${minizip_SOURCE_DIR}/mz.h
    ${minizip_SOURCE_DIR}/mz_os.h
    ${minizip_SOURCE_DIR}/mz_crypt.h
    ${minizip_SOURCE_DIR}/mz_strm.h
    ${minizip_SOURCE_DIR}/mz_strm_buf.h
    ${minizip_SOURCE_DIR}/mz_strm_mem.h
    ${minizip_SOURCE_DIR}/mz_strm_split.h
    ${minizip_SOURCE_DIR}/mz_strm_os.h
    ${minizip_SOURCE_DIR}/mz_zip.h
    ${minizip_SOURCE_DIR}/mz_strm_zstd.h

    # Built 
    ${minizip_BINARY_DIR}/unzip.h

    # ZSTD
    ${minizip_SOURCE_DIR}/mz_strm_zstd.h
)

set(MINIZIP_SRC ${MINIZIP_SOURCES} ${MINIZIP_HEADERS} PARENT_SCOPE)
set(MINIZIP_HEADERS ${MINIZIP_HEADERS} CACHE INTERNAL "")
set(MINIZIP_INCLUDE_DIR ${minizip_BINARY_DIR} ${minizip_SOURCE_DIR} CACHE INTERNAL "")

include(FFMPEGHandling.cmake)
