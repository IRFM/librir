# Generated from librir.pro.

cmake_minimum_required(VERSION 3.16)

project(librir
    VERSION 6.0.0
    DESCRIPTION "librir"
    HOMEPAGE_URL "https://github.com/IRFM/librir"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(GenerateExportHeader)

# option(LOCAL_INSTALL "Install locally" ON)
set(FFMPEG_VERSION_REQUIRED "7.1" CACHE STRING "FFmpeg version to use")

if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(NOT MSVC)
        set(CMAKE_INSTALL_RPATH $ORIGIN)

        set(CMAKE_SKIP_BUILD_RPATH FALSE)
        set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    endif()
endif()

option(ENABLE_H264 "Enable h264 capability for compression" ON)

# TODO: get rid of these definitions. Either set them directly in code if mandatory
# or set them as options in this file.
add_definitions("-DUNICODE")
add_definitions("-D_OPENMP")

option(LIBRIR_ENABLE_ZSTD "Enable ZSTD support" ON)

if(LIBRIR_ENABLE_ZSTD)
    add_compile_definitions(HAVE_ZSTD)
    add_compile_definitions(ZSTD_COMPRESS)
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX)
    add_compile_definitions(WIN64)
    add_compile_definitions(DISABLE_WINRT_DEPRECATION)
    add_compile_definitions(_WINDOWS)
endif()

configure_file(rir_config.h.in rir_config.h)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rir_config.h DESTINATION include)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rir_config.h DESTINATION include/api)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# for rir_config.h needed by all targets
# include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(RIR_CONFIG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/rir_config.h CACHE INTERNAL "")

# get_filename_component(RIR_CONFIG_DIR ${RIR_CONFIG_HEADER} DIRECTORY )
set(RIR_CONFIG_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "")
include_directories(${RIR_CONFIG_DIR})

# get FFMPEG
option(BUILD_FFMPEG "build FFMPEG from source" ON)

if(MSVC AND NOT FFMPEG_DIR)
    set(BUILD_FFMPEG OFF)
endif()

add_subdirectory(extra)

# Pursue compilation
add_subdirectory(src)
add_subdirectory(tests)

# Generate requirements.txt
configure_file(requirements.txt.in requirements.txt @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt DESTINATION python)

# for FFMPEG and additional artifacts
install(IMPORTED_RUNTIME_ARTIFACTS video_io geometry signal_processing tools
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

include(CMakePackageConfigHelpers)

configure_package_config_file(cmake/librirConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/librirConfig.cmake
    INSTALL_DESTINATION lib/cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/librirConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

set(ConfigPackageLocation lib/cmake/librir)

install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/librirConfig.cmake"
    DESTINATION lib/cmake/librir
)

# Generate librir.pc
list(TRANSFORM PLUGINS_LIBS PREPEND "-l")
configure_file(librir.pc.in librir.pc @ONLY)
configure_file(librir.pc.in ${CMAKE_INSTALL_LIBDIR}/pkgconfig/librir.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/librir.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
