# Generated from librir.pro.

cmake_minimum_required(VERSION 3.16)

project(librir
	VERSION 5.1.2
	DESCRIPTION "librir"
	HOMEPAGE_URL "https://github.com/IRFM/librir"
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(GenerateExportHeader)

# option(LOCAL_INSTALL "Install locally" ON)
set(FFMPEG_VERSION_REQUIRED "4.3" CACHE STRING "FFmpeg version to use")

IF(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	IF(NOT MSVC)
		SET(CMAKE_INSTALL_RPATH $ORIGIN)

		SET(CMAKE_SKIP_BUILD_RPATH FALSE)
		SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
		SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	ENDIF()
ENDIF()

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

IF(WIN32)
	add_compile_definitions(NOMINMAX)
	add_compile_definitions(WIN64)
	add_compile_definitions(DISABLE_WINRT_DEPRECATION)
	add_compile_definitions(_WINDOWS)
ENDIF()

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

# install(
# TARGETS ${LIBRIR_TARGETS}
# PUBLIC_HEADER
# LIBRARY COMPONENT Runtime
# ARCHIVE COMPONENT Development
# RUNTIME COMPONENT Runtime
# )

# for FFMPEG and additional artifacts
install(IMPORTED_RUNTIME_ARTIFACTS video_io geometry signal_processing tools
	DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Configure and install PackageConfig files
# generate_export_header(librir)
# set_property(TARGET librir PROPERTY VERSION ${PROJECT_VERSION})
# set_property(TARGET librir PROPERTY SOVERSION ${PROJECT_VERSION_MAJOR})
# set_property(TARGET librir PROPERTY
# INTERFACE_librir_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
# set_property(TARGET librir APPEND PROPERTY
# COMPATIBLE_INTERFACE_STRING librir_MAJOR_VERSION
# )
# install(TARGETS ${LIBRIR_TARGETS}
# LIBRARY DESTINATION lib
# ARCHIVE DESTINATION lib
# RUNTIME DESTINATION bin
# INCLUDES DESTINATION include
# )

# install(
# TARGETS ${LIBRIR_TARGETS}
# EXPORT LibrirTargets
# )
include(CMakePackageConfigHelpers)

configure_package_config_file(cmake/librirConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/librirConfig.cmake
	INSTALL_DESTINATION lib/cmake
)

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/librirConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion)

# export(EXPORT LibrirTargets
# FILE "${CMAKE_CURRENT_BINARY_DIR}/librirConfig.cmake"
# NAMESPACE librir::
# )
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
