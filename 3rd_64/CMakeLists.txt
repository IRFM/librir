cmake_minimum_required(VERSION 3.16)

project(ffmpeg_librir
    VERSION 	7.1
    DESCRIPTION "ffmpeg_librir"
    
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)
include(FetchContent)

set(3RD_64_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
option(FFMPEG_LOCAL_INSTALL "Install locally" OFF)
IF(FFMPEG_LOCAL_INSTALL)
    set(3RD_64_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)
endif()

option(BUILD_OFFLINE "Build offline" OFF)
MESSAGE(STATUS ${CMAKE_SOURCE_DIR})
if (${BUILD_OFFLINE})
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                 PROPERTY EP_UPDATE_DISCONNECTED 1)
else()
# set(EP_DOWNLOAD_COMMAND DOWNLOAD_COMMAND 1)

endif()

FetchContent_Declare(
    nasm
    URL https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2
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
    kvazaar-librir
    GIT_REPOSITORY https://github.com/ultravideo/kvazaar.git
    GIT_TAG v2.3.1
    )
FetchContent_MakeAvailable(kvazaar-librir)
set(KVAZAAR_CONFIGURE_CMD sed -i "/AM_PROG_AR/c\\m4_ifdef([AM_PROG_AR], [AM_PROG_AR])" ./configure.ac  && ./autogen.sh &&  ./configure --enable-static=yes --enable-shared=no CFLAGS=-fPIC )#--extra-ldflags="-Wl,-rpath='$ORIGIN'")
ExternalProject_Add(kvazaar-librir
                    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/kvazaar-librir-src
                    UPDATE_DISCONNECTED true
                    DOWNLOAD_EXTRACT_TIMESTAMP true
                    CMAKE_ARGS  
                        -DBUILD_KVAZAAR_BINARY=OFF
                        -DBUILD_TESTS=OFF
                        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                        -DCMAKE_INSTALL_LIBDIR=<INSTALL_DIR>/lib
                        # -DCMAKE_INSTALL_BINDIR=<INSTALL_DIR>/bin
                        # -DCMAKE_INSTALL_INCLUDEDIR=<INSTALL_DIR>/include
                        # -DCMAKE_INSTALL_MANDIR=<INSTALL_DIR>/man
                        -DKVZ_DEFAULT_BUILD_TYPE=Release
                    # PREFIX deps
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    # CONFIGURE_COMMAND ${KVAZAAR_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    # BUILD_COMMAND $(MAKE) --silent
                    # INSTALL_COMMAND ${KVAZAAR_INSTALL_CMD}
                    BUILD_IN_SOURCE true
                    DOWNLOAD_COMMAND ""
                    # SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/kvazaar-prefix/src
                    )

add_custom_command(
                        TARGET kvazaar-librir
                        POST_BUILD 
                        COMMAND bash -c "sed -i \"/lkvazaar/c $(grep lkvazaar ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc) -lm -lpthread\" ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc"
                        VERBATIM
                      )
ExternalProject_Add_StepTargets(kvazaar-librir build)
list( APPEND CMAKE_INSTALL_RPATH "../../out" )

FetchContent_Declare(
    x264
    GIT_REPOSITORY https://code.videolan.org/videolan/x264.git
    GIT_TAG stable
    )
FetchContent_MakeAvailable(x264)
set(_PATH $ENV{PATH})
set(X264_CONFIGURE_CMD  export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && ./configure --enable-static --enable-pic --enable-strip )
set(X264_BUILD_CMD  export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && $(MAKE) )
ExternalProject_Add(x264
                    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/x264-src
                    UPDATE_DISCONNECTED true
                    # DOWNLOAD_EXTRACT_TIMESTAMP true
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ${X264_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND ${X264_BUILD_CMD}
                    DEPENDS nasm
                    BUILD_IN_SOURCE true
                    DOWNLOAD_COMMAND ""
)
ExternalProject_Add_StepTargets(x264 build)

set(_PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH})

FetchContent_Declare(
    ffmpeg
    GIT_REPOSITORY https://git.ffmpeg.org/ffmpeg.git
    GIT_TAG release/${PROJECT_VERSION}
    )
FetchContent_MakeAvailable(ffmpeg)
set(FFMPEG_CONFIGURE_CMD    
                            export PATH=${3RD_64_INSTALL_DIR}/bin:$ENV{PATH}  && 
                            export LD_LIBRARY_PATH=${3RD_64_INSTALL_DIR}/lib:$ENV{LD_LIBRARY_PATH}  && 
                            export PKG_CONFIG_PATH=${3RD_64_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}   &&
                            export OPENSSL_LIBS="-L/usr/lib64 -llibeay32 -lssleay32 -llibcrypto -luser32 -lgdi32 -lwsock32 -lws2_32" && 
                            ./configure --arch=native --enable-asm --disable-libvpx --disable-libaom --disable-static --enable-shared --enable-libx264 --enable-libkvazaar --extra-cflags=-DKVZ_STATIC_LIB  --enable-gpl --enable-rpath --enable-pic --disable-debug)

ExternalProject_Add(ffmpeg
                    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/ffmpeg-src
                    DOWNLOAD_EXTRACT_TIMESTAMP true
                    UPDATE_DISCONNECTED true
                    CONFIGURE_COMMAND ${FFMPEG_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND $(MAKE)
                    INSTALL_COMMAND make install
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    BUILD_IN_SOURCE true
                    DEPENDS kvazaar-librir
                    DEPENDS x264
                    DOWNLOAD_COMMAND ""
                    # BUILD_ALWAYS 1
)