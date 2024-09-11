cmake_minimum_required(VERSION 3.16)

project(ffmpeg_librir
    VERSION 4.3
    DESCRIPTION "ffmpeg_librir"
    
    LANGUAGES CXX C
)


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)


set(3RD_64_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)

ExternalProject_Add(nasm
                    URL https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/nasm-2.15.05.tar.bz2
                    DOWNLOAD_EXTRACT_TIMESTAMP true
                    
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR>
                    BUILD_COMMAND $(MAKE) 
                    # INSTALL_COMMAND $(MAKE) install
                    BUILD_IN_SOURCE true
)


list(APPEND ffmpeg_DEPENDS nasm)
set(KVAZAAR_CONFIGURE_CMD sed -i "/AM_PROG_AR/c\\m4_ifdef([AM_PROG_AR], [AM_PROG_AR])" ./configure.ac  && ./autogen.sh &&  ./configure --enable-static=yes --enable-shared=no CFLAGS=-fPIC )#--extra-ldflags="-Wl,-rpath='$ORIGIN'")
set(KVAZAAR_INSTALL_CMD $(MAKE) install && bash -c export line=$(grep lkvazaar ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc) && export line="${line} -lm -lpthread" && sed -i "/lkvazaar/c ${line}" ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc)
ExternalProject_Add(kvazaar
                    GIT_REPOSITORY https://github.com/ultravideo/kvazaar.git
                    GIT_TAG v2.2.0
                    # PREFIX deps
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ${KVAZAAR_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND $(MAKE) --silent
                    # INSTALL_COMMAND ${KVAZAAR_INSTALL_CMD}
                    BUILD_IN_SOURCE 1
                    )

add_custom_command(
                        TARGET kvazaar
                        POST_BUILD 
                        COMMAND bash -c "sed -i \"/lkvazaar/c $(grep lkvazaar ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc) -lm -lpthread\" ${3RD_64_INSTALL_DIR}/lib/pkgconfig/kvazaar.pc"
                        VERBATIM
                      )

set(_PATH $ENV{PATH})
set(X264_CONFIGURE_CMD  export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && ./configure --enable-static --enable-pic --enable-strip )
set(X264_BUILD_CMD  export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && $(MAKE) )
ExternalProject_Add(x264
                    GIT_REPOSITORY https://code.videolan.org/videolan/x264.git
                    GIT_TAG stable
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ${X264_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND ${X264_BUILD_CMD}
                    # INSTALL_COMMAND $(MAKE) install
                    DEPENDS nasm
                    BUILD_IN_SOURCE 1
)

# ExternalProject_Add_StepDependencies(x264 configure ${3RD_64_INSTALL_DIR}/bin/nasm)
# ExternalProject_Add_Step(x264 configure DEPENDEES nasm-install)
list(APPEND ffmpeg_DEPENDS x264)

set(_PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH})

# # )
set(FFMPEG_CONFIGURE_CMD    
                            export PATH=${3RD_64_INSTALL_DIR}/bin:$ENV{PATH}  && 
                            export LD_LIBRARY_PATH=${3RD_64_INSTALL_DIR}/lib:$ENV{LD_LIBRARY_PATH}  && 
                            export PKG_CONFIG_PATH=${3RD_64_INSTALL_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}   &&
                            export OPENSSL_LIBS="-L/usr/lib64 -llibeay32 -lssleay32 -llibcrypto -luser32 -lgdi32 -lwsock32 -lws2_32" && 
                            # export LDFLAGS="-lm":$ENV{LDFLAGS} && 
                            ./configure --arch=native --enable-asm --disable-libvpx --disable-libaom --disable-static --enable-shared --enable-libx264 --enable-libkvazaar --extra-cflags=-DKVZ_STATIC_LIB  --enable-gpl --enable-rpath --enable-pic --disable-debug)

ExternalProject_Add(ffmpeg
                    GIT_REPOSITORY https://git.ffmpeg.org/ffmpeg.git
                    GIT_TAG n4.3
                    CONFIGURE_COMMAND ${FFMPEG_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND $(MAKE)
                    INSTALL_COMMAND make install
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    BUILD_IN_SOURCE 1
                    DEPENDS kvazaar
                    DEPENDS x264
                    # BUILD_ALWAYS 1
)


# ExternalProject_Add_StepDependencies(ffmpeg build nasm kvazaar x264)