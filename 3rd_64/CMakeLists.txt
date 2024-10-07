cmake_minimum_required(VERSION 3.16)

project(ffmpeg_librir
    VERSION 	4.3
    DESCRIPTION "ffmpeg_librir"
    
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)
set(3RD_64_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})
option(FFMPEG_LOCAL_INSTALL "Install locally" OFF)
IF(FFMPEG_LOCAL_INSTALL)
    set(3RD_64_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)
endif()

ExternalProject_Add(nasm
                    URL https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2
                    DOWNLOAD_EXTRACT_TIMESTAMP true
                    
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR>
                    BUILD_COMMAND $(MAKE) 
                    # INSTALL_COMMAND $(MAKE) install
                    BUILD_IN_SOURCE true
)
ExternalProject_Add_StepTargets(nasm install)


set(KVAZAAR_CONFIGURE_CMD sed -i "/AM_PROG_AR/c\\m4_ifdef([AM_PROG_AR], [AM_PROG_AR])" ./configure.ac  && ./autogen.sh &&  ./configure --enable-static=yes --enable-shared=no CFLAGS=-fPIC )#--extra-ldflags="-Wl,-rpath='$ORIGIN'")
ExternalProject_Add(kvazaar
                    GIT_REPOSITORY https://github.com/ultravideo/kvazaar.git
                    UPDATE_DISCONNECTED true
                    DOWNLOAD_EXTRACT_TIMESTAMP true
                    GIT_TAG v2.2.0
                    # PREFIX deps
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ${KVAZAAR_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND $(MAKE) --silent
                    # INSTALL_COMMAND ${KVAZAAR_INSTALL_CMD}
                    BUILD_IN_SOURCE true
                    )

add_custom_command(
                        TARGET kvazaar
                        POST_BUILD 
                        COMMAND bash -c "sed -i \"/lkvazaar/c $(grep lkvazaar ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc) -lm -lpthread\" ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc"
                        VERBATIM
                      )
ExternalProject_Add_StepTargets(kvazaar build)

set(_PATH $ENV{PATH})
set(X264_CONFIGURE_CMD  export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && ./configure --enable-static --enable-pic --enable-strip )
set(X264_BUILD_CMD  export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && $(MAKE) )
ExternalProject_Add(x264
                    GIT_REPOSITORY https://code.videolan.org/videolan/x264.git
                    GIT_TAG stable
                    UPDATE_DISCONNECTED true
                    # DOWNLOAD_EXTRACT_TIMESTAMP true
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ${X264_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND ${X264_BUILD_CMD}
                    DEPENDS nasm
                    BUILD_IN_SOURCE true
)
ExternalProject_Add_StepTargets(x264 build)

set(_PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH})

set(FFMPEG_CONFIGURE_CMD    
                            export PATH=${3RD_64_INSTALL_DIR}/bin:$ENV{PATH}  && 
                            export LD_LIBRARY_PATH=${3RD_64_INSTALL_DIR}/lib:$ENV{LD_LIBRARY_PATH}  && 
                            export PKG_CONFIG_PATH=${3RD_64_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}   &&
                            export OPENSSL_LIBS="-L/usr/lib64 -llibeay32 -lssleay32 -llibcrypto -luser32 -lgdi32 -lwsock32 -lws2_32" && 
                            ./configure --arch=native --enable-asm --disable-libvpx --disable-libaom --disable-static --enable-shared --enable-libx264 --enable-libkvazaar --extra-cflags=-DKVZ_STATIC_LIB  --enable-gpl --enable-rpath --enable-pic --disable-debug)

ExternalProject_Add(ffmpeg
                    GIT_REPOSITORY https://git.ffmpeg.org/ffmpeg.git
                    GIT_TAG 	release/${PROJECT_VERSION}
                    DOWNLOAD_EXTRACT_TIMESTAMP true
                    UPDATE_DISCONNECTED true
                    CONFIGURE_COMMAND ${FFMPEG_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND $(MAKE)
                    INSTALL_COMMAND make install
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    BUILD_IN_SOURCE true
                    DEPENDS kvazaar
                    DEPENDS x264
                    # BUILD_ALWAYS 1
)