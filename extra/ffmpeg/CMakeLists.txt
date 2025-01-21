cmake_minimum_required(VERSION 3.16)

project(ffmpeg_librir
    VERSION 4.3
    DESCRIPTION "ffmpeg_librir"

    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)
include(FetchContent)

set(3RD_64_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
option(FFMPEG_LOCAL_INSTALL "Install locally" ON)

IF(FFMPEG_LOCAL_INSTALL)
    set(3RD_64_INSTALL_DIR ${CMAKE_BINARY_DIR}/3rd-party-install)
endif()

set(3RD_64_INSTALL_DIR ${3RD_64_INSTALL_DIR} CACHE INTERNAL "")

option(BUILD_OFFLINE "Build offline" OFF)

# MESSAGE(STATUS ${CMAKE_SOURCE_DIR})
if(BUILD_OFFLINE)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTY EP_UPDATE_DISCONNECTED 1)
else()
    # set(EP_DOWNLOAD_COMMAND DOWNLOAD_COMMAND 1)
endif()

cmake_policy(SET CMP0114 NEW)
cmake_policy(SET CMP0135 NEW)

FetchContent_Declare(
    nasm
    URL https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2
    EXCLUDE_FROM_ALL
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(nasm)
ExternalProject_Add(nasm
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/nasm-src
    DOWNLOAD_EXTRACT_TIMESTAMP true
    UPDATE_DISCONNECTED true
    INSTALL_DIR ${3RD_64_INSTALL_DIR}
    CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND $(MAKE)

    # INSTALL_COMMAND $(MAKE) install
    BUILD_IN_SOURCE true
    DOWNLOAD_COMMAND ""
)
ExternalProject_Add_StepTargets(nasm install)

FetchContent_Declare(
    kvazaar
    GIT_REPOSITORY https://github.com/ultravideo/kvazaar.git
    GIT_TAG v2.2.0
    EXCLUDE_FROM_ALL
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(kvazaar)
set(KVAZAAR_CONFIGURE_CMD sed -i "/AM_PROG_AR/c\\m4_ifdef([AM_PROG_AR], [AM_PROG_AR])" ./configure.ac && ./autogen.sh && ./configure --enable-static=yes --enable-shared=no CFLAGS=-fPIC) # --extra-ldflags="-Wl,-rpath='$ORIGIN'")
ExternalProject_Add(kvazaar
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/kvazaar-src
    UPDATE_DISCONNECTED true
    DOWNLOAD_EXTRACT_TIMESTAMP true

    # PREFIX deps
    INSTALL_DIR ${3RD_64_INSTALL_DIR}
    CONFIGURE_COMMAND ${KVAZAAR_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
    BUILD_COMMAND $(MAKE) --silent
    BUILD_IN_SOURCE true
    DOWNLOAD_COMMAND ""
)

add_custom_command(
    TARGET kvazaar
    POST_BUILD
    COMMAND bash -c "sed -i \"/lkvazaar/c $(grep lkvazaar ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc) -lm -lpthread\" ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc"
    VERBATIM
)
ExternalProject_Add_StepTargets(kvazaar build install)

FetchContent_Declare(
    x264
    GIT_REPOSITORY https://code.videolan.org/videolan/x264.git
    GIT_TAG stable
    EXCLUDE_FROM_ALL
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(x264)
set(_PATH $ENV{PATH})
set(X264_CONFIGURE_CMD export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && ./configure --enable-static --enable-pic --enable-strip)
set(X264_BUILD_CMD export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && $(MAKE))
ExternalProject_Add(x264
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/x264-src
    UPDATE_DISCONNECTED true
    DOWNLOAD_EXTRACT_TIMESTAMP true
    INSTALL_DIR ${3RD_64_INSTALL_DIR}
    CONFIGURE_COMMAND ${X264_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
    BUILD_COMMAND ${X264_BUILD_CMD}
    DEPENDS nasm-install
    BUILD_IN_SOURCE true
    DOWNLOAD_COMMAND ""
)
ExternalProject_Add_StepTargets(x264 build)

set(_PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH})

FetchContent_Declare(
    ffmpeg
    GIT_REPOSITORY https://git.ffmpeg.org/ffmpeg.git
    GIT_TAG release/${PROJECT_VERSION}
    EXCLUDE_FROM_ALL
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(ffmpeg)
set(FFMPEG_CONFIGURE_CMD
    export PATH=${3RD_64_INSTALL_DIR}/bin:$ENV{PATH} &&
    export LD_LIBRARY_PATH=${3RD_64_INSTALL_DIR}/lib:$ENV{LD_LIBRARY_PATH} &&
    export PKG_CONFIG_PATH=${3RD_64_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH} &&
    export OPENSSL_LIBS="-L/usr/lib64 -llibeay32 -lssleay32 -llibcrypto -luser32 -lgdi32 -lwsock32 -lws2_32" &&
    ./configure --arch=native --enable-asm --disable-libvpx --disable-libaom --disable-static --enable-shared --enable-libx264 --enable-libkvazaar --extra-cflags=-DKVZ_STATIC_LIB --enable-gpl --enable-rpath --enable-pic --disable-debug)

ExternalProject_Add(ffmpeg
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/ffmpeg-src
    DOWNLOAD_EXTRACT_TIMESTAMP true
    UPDATE_DISCONNECTED true
    CONFIGURE_COMMAND ${FFMPEG_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
    BUILD_COMMAND $(MAKE)

    INSTALL_DIR ${3RD_64_INSTALL_DIR}
    BUILD_IN_SOURCE true
    DEPENDS kvazaar
    DEPENDS x264
    DOWNLOAD_COMMAND ""
    EXCLUDE_FROM_ALL
)

set(FFMPEG_INCLUDE_DIRS ${3RD_64_INSTALL_DIR}/include CACHE INTERNAL "" FORCE)
set(FFMPEG_LIB_DIR ${3RD_64_INSTALL_DIR}/lib CACHE INTERNAL "" FORCE)

file(MAKE_DIRECTORY ${FFMPEG_INCLUDE_DIRS})
file(MAKE_DIRECTORY ${FFMPEG_LIB_DIR})

set(FFMPEG_SHARED_LIBRARIES_VERSIONS
    58.91.100 # libavcodec
    58.10.100 # libavdevice
    7.85.100 # libavfilter
    58.45.100 # libavformat
    56.51.100 # libavutil
    3.7.100 # libswresample
    5.7.100 # libswscale
    55.7.100 # libpostproc
)
set(FFMPEG_SHARED_LIBRARIES)

foreach(component version IN ZIP_LISTS REQUIRED_FFMPEG_COMPONENTS FFMPEG_SHARED_LIBRARIES_VERSIONS)
    add_library("FFMPEG::lib${component}" SHARED IMPORTED GLOBAL)
    add_dependencies("FFMPEG::lib${component}" ffmpeg)

    set(${component}_LIBRARY_NAME lib${component}.so)

    string(REPLACE "." ";" VERSION_LIST ${version})
    list(GET VERSION_LIST 0 ${component}_VERSION_MAJOR)

    set(${component}_LIBRARY_SONAME lib${component}.so.${${component}_VERSION_MAJOR})
    # set(${component}_MAJOR_SOVERSION_LIBRARY ${FFMPEG_LIB_DIR}/lib${component}.so.${${component}_VERSION_MAJOR})
    set(ACTUAL_${component}_LIBRARY ${FFMPEG_LIB_DIR}/lib${component}.so.${version})

    set_target_properties("FFMPEG::lib${component}" PROPERTIES
        IMPORTED_LOCATION "${ACTUAL_${component}_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES ${FFMPEG_INCLUDE_DIRS}
        IMPORTED_SONAME "${${component}_LIBRARY_SONAME}"
    )
    # get_target_property(${component}_SOVERSION "FFMPEG::lib${component}" SOVERSION )
    message(STATUS "${component}_SOVERSION : ${${component}_LIBRARY_SONAME}")

    list(APPEND FFMPEG_SHARED_LIBRARIES FFMPEG::lib${component})
endforeach()

set(FFMPEG_SHARED_LIBRARIES ${FFMPEG_SHARED_LIBRARIES} CACHE INTERNAL "")
