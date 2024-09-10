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
# ExternalProject_Add_Step(nasm build BYPRODUCTS <INSTALL_DIR>/nasm)
# ExternalProject_Add_StepTargets(nasm build)
# ExternalProject_Add_StepTargets(nasm install)

list(APPEND ffmpeg_DEPENDS nasm)

ExternalProject_Add(kvazaar
                    GIT_REPOSITORY https://github.com/ultravideo/kvazaar.git
                    GIT_TAG v2.2.0
                    # PREFIX deps
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ./autogen.sh && ./configure --enable-static=yes --enable-shared=no CFLAGS=-fPIC --prefix=<INSTALL_DIR> #--extra-ldflags="-Wl,-rpath='$ORIGIN'"
                    BUILD_COMMAND $(MAKE) --silent
                    # INSTALL_COMMAND $(MAKE) install
                    BUILD_IN_SOURCE 1
                    )



# add_executable(nasm-exec IMPORTED)
# set_property(
#             TARGET nasm-exec
#             PROPERTY
#                 IMPORTED_LOCATION "${3RD_64_INSTALL_DIR}/bin/nasm"
#         )
set(_PATH $ENV{PATH})
set(X264_CONFIGURE_CMD  export PATH=${3RD_64_INSTALL_DIR}/bin:${_PATH} && ./configure --enable-static --enable-pic --enable-strip )
ExternalProject_Add(x264
                    GIT_REPOSITORY https://code.videolan.org/videolan/x264.git
                    # GIT_TAG v2.2.0
                    INSTALL_DIR ${3RD_64_INSTALL_DIR}
                    CONFIGURE_COMMAND ${X264_CONFIGURE_CMD} --prefix=<INSTALL_DIR>
                    BUILD_COMMAND $(MAKE)
                    # INSTALL_COMMAND $(MAKE) install
                    BUILD_IN_SOURCE 1
)

# ExternalProject_Add_StepDependencies(x264 configure ${3RD_64_INSTALL_DIR}/bin/nasm)
# ExternalProject_Add_Step(x264 configure DEPENDEES nasm-install)
list(APPEND ffmpeg_DEPENDS x264)

# # )


# ExternalProject_Add(ffmpeg
#                     GIT_REPOSITORY https://git.ffmpeg.org/ffmpeg.git
#                     GIT_TAG n4.3
#                     CONFIGURE_COMMAND export OPENSSL_LIBS="-L /usr/lib64 -llibeay32 -lssleay32 -llibcrypto -luser32 -lgdi32 -lwsock32 -lws2_32";./configure --arch=native --enable-asm --disable-libvpx --disable-libaom --disable-static --enable-shared --enable-libx264 --enable-libkvazaar --extra-cflags=-DKVZ_STATIC_LIB --prefix=$PWD/install --enable-gpl --enable-rpath --extra-ldflags="-L/usr/local/lib -ldl $ffmeg_extra_ldflags -fPIC -m64 -Wl,-rpath='$ORIGIN'" --enable-pic --disable-debug
#                     BUILD_COMMAND export LD_LIBRARY_PATH=${PRIVATE_LIBRARY_PATH} PKG_CONFIG_PATH=${PRIVATE_PKG_CONFIG}/usr/lib/pkgconfig && $(MAKE) V=1
#                     INSTALL_COMMAND make install
#                     INSTALL_DIR ""
#                     BUILD_IN_SOURCE 1
#                     BUILD_ALWAYS 1
# )


# ExternalProject_Add_StepDependencies(ffmpeg build nasm kvazaar x264)