# Generated from librir.pro.

cmake_minimum_required(VERSION 3.16)

project(librir
    VERSION 4.0.0
    DESCRIPTION "librir"
    HOMEPAGE_URL "https://github.com/IRFM/librir"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs) 

option(LOCAL_INSTALL "Install locally" ON)
IF(LOCAL_INSTALL)
	SET(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install) 
	MESSAGE(STATUS ${CMAKE_INSTALL_PREFIX})
	set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out) 
	set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

	IF(NOT MSVC)
		SET(CMAKE_INSTALL_RPATH $ORIGIN)
		
		SET(CMAKE_SKIP_BUILD_RPATH FALSE)
		SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
		#~ SET(CMAKE_INSTALL_RPATH_USE_ORIGIN TRUE)
		SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	ENDIF()
ENDIF()


IF (UNIX)
	execute_process(COMMAND "chmod 777 configure_ffmpeg")
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
install(FILES ${CMAKE_BINARY_DIR}/rir_config.h DESTINATION include)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
# set(THREADS_PREFER_PTHREAD_FLAG ON)
add_subdirectory(extra)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})


# # DOCTEST
# set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_BINARY_DIR}/doctest/doctest.h)
# set(doctest_FOUND FALSE)
# # Expose required variable (DOCTEST_INCLUDE_DIR) to parent scope
# set(DOCTEST_INCLUDE_DIR ${CMAKE_BINARY_DIR}/doctest/src/doctest CACHE INTERNAL "Path to include folder for doctest")
# IF(EXISTS ${DOCTEST_INCLUDE_DIR}/doctest/doctest.h)
	# set(doctest_FOUND TRUE)
# endif()
# MESSAGE(STATUS ${doctest_FOUND})
# if(NOT doctest_FOUND)
	# include(ExternalProject)
	# find_package(Git)
	# if(Git_FOUND)
		# ExternalProject_Add(
			# doctest
			# PREFIX ${CMAKE_BINARY_DIR}/doctest
			# GIT_REPOSITORY https://github.com/onqtam/doctest.git
			# TIMEOUT 10
			# UPDATE_COMMAND ${GIT_EXECUTABLE} pull
			# CONFIGURE_COMMAND ""
			# BUILD_COMMAND ""
			# INSTALL_COMMAND ""
			# LOG_DOWNLOAD ON
		# )
	# endif()
# endif()
# include_directories(${DOCTEST_INCLUDE_DIR})

# list(APPEND TARGETS ${PRESENT_PLUGINS})
# message(STATUS "PRESENT_PLUGINS : " ${PRESENT_PLUGINS})
# message(STATUS "TARGETS : " ${TARGETS})
# add it globally


# Create global variable used in librir.pc.in
# Plugins can add libraries to link with for generated librir.pc
set(PLUGINS_LIBS "" CACHE INTERNAL "")

set(PLUGINS_PYTHON_REQUIRE "" CACHE INTERNAL "")

add_subdirectory(src)
add_subdirectory(plugins)
#add_subdirectory(tests)

# enable_testing()
# TOREMOVE

find_package(Python COMPONENTS Interpreter)
IF(NOT Python_FOUND)

	MESSAGE(STATUS "PYTHON NOT FOUND")

endif() 

MESSAGE(STATUS "found python")


# Perform this action once more (already done in src/CMakeLists.txt) to copy plugins binaries
install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/
		DESTINATION ${CMAKE_INSTALL_PREFIX}/librir/libs
)

# Generate requirements.txt
configure_file(requirements.txt.in requirements.txt @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/requirements.txt DESTINATION ${CMAKE_INSTALL_PREFIX})


#set(INCLUDE_DIR "include" CACHE PATH "Location of header files" )
#set(LIB_DIR "lib lib64" CACHE PATH "Location of libraries" )
#set(LIBRARIES "tools geometry signal_processing video_io" CACHE PATH "Libraries" )

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/librirConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/librirConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/librir
  PATH_VARS 
  PLUGINS_LIBS)
  #INCLUDE_DIR LIB_DIR LIBRARIES PLUGINS_LIBS)
  
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
install(FILES ${CMAKE_BINARY_DIR}/librir.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

