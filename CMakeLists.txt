# Generated from librir.pro.

cmake_minimum_required(VERSION 3.16)

project(librir
    VERSION 5.0.0
    DESCRIPTION "librir"
    HOMEPAGE_URL "https://github.com/IRFM/librir"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(LOCAL_INSTALL "Install locally" ON)

IF(LOCAL_INSTALL)
	SET(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/install) 
	MESSAGE(STATUS ${CMAKE_INSTALL_PREFIX})
	set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out) 
	set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
	
	IF(NOT MSVC)
		SET(CMAKE_INSTALL_RPATH $ORIGIN)
		
		SET(CMAKE_SKIP_BUILD_RPATH FALSE)
		SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
		SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	ENDIF()
ENDIF()
include(GNUInstallDirs) 

IF (UNIX)
	execute_process(COMMAND "chmod 100755 configure_ffmpeg")
ENDIF()

IF (NOT MSVC)
	# Create a dummy target to build ffmpeg as a prebuild step
	add_custom_target(ffmpeg BYPRODUCTS "${PROJECT_SOURCE_DIR}/build_ffmpeg.h"
	COMMAND ${PROJECT_SOURCE_DIR}/configure_ffmpeg)
	add_executable(dummy ${PROJECT_SOURCE_DIR}/dummy.c "${PROJECT_SOURCE_DIR}/build_ffmpeg.h")
	target_include_directories(dummy PRIVATE "${PROJECT_SOURCE_DIR}")
	
ENDIF()


add_definitions("-DUNICODE")
add_definitions("-D_OPENMP")
add_definitions("-DENABLE_H264")
add_definitions("-DHAVE_ZSTD")
add_definitions("-DZSTD_COMPRESS")

IF (WIN32)
	add_definitions("-DNOMINMAX")
	add_definitions("-DWIN64")
	add_definitions("-DDISABLE_WINRT_DEPRECATION")
	add_definitions("-D_WINDOWS")
ENDIF()



configure_file(rir_config.h.in rir_config.h)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rir_config.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# TODO: move all elements in extra folder to dedicated FetchContent/find_package modules
add_subdirectory(extra)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Create global variable used in librir.pc.in
# Plugins can add libraries to link with for generated librir.pc
set(PLUGINS_LIBS "" CACHE INTERNAL "")

set(PLUGINS_PYTHON_REQUIRE "" CACHE INTERNAL "")

# Pusue compilation
add_subdirectory(src)
add_subdirectory(tests)

# Generate requirements.txt
configure_file(requirements.txt.in requirements.txt @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/requirements.txt DESTINATION ${CMAKE_INSTALL_PREFIX})


install(
	TARGETS ${LIBRIR_TARGETS} 
	PUBLIC_HEADER
	ARCHIVE
	RUNTIME
	)
install(
	TARGETS ${LIBRIR_TARGETS} 
	EXPORT LibrirTargets
	)
install(EXPORT LibrirTargets
		FILE LibrirTargets.cmake 
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/librir
		NAMESPACE librir::
)
		

# Configure and install PackageConfig files
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/librirConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/librirConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)
  
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/librirConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion )
  
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/librirConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/librirConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/librir )




# Generate librir.pc
list(TRANSFORM PLUGINS_LIBS PREPEND "-l")
configure_file(librir.pc.in librir.pc @ONLY)
configure_file(librir.pc.in ${CMAKE_INSTALL_LIBDIR}/pkgconfig/librir.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/librir.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

