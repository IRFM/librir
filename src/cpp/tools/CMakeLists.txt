# Find or fetch ZSTD
#include(FetchContent)

#set(TOOLS_DEP_LIBS)
#message(STATUS "fetching ZSTD")
#set(ZSTD_BUILD_TESTS OFF CACHE INTERNAL "")  # Forces the value
#set(ZSTD_BUILD_SHARED ON CACHE INTERNAL "")  # Forces the value
#set(ZSTD_BUILD_PROGRAMS OFF CACHE INTERNAL "")  # Forces the value
#set(ZSTD_BUILD_SHARED OFF CACHE INTERNAL "")  # Forces the value
#FetchContent_Declare(
#	zstd
#	GIT_REPOSITORY 	"https://github.com/facebook/zstd.git"
#	GIT_TAG			v1.5.6
#	# URL "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz"
#	SOURCE_SUBDIR build/cmake
#	#OVERRIDE_FIND_PACKAGE
#	)

#FetchContent_MakeAvailable(zstd)

#find_package(zstd REQUIRED)

#set(ZSTD_INCLUDE_DIR ${zstd_SOURCE_DIR}/lib)
#list(APPEND TOOLS_DEP_LIBS zstd::libzstd_shared)
#get_target_property(ZSTD_INCLUDE_DIR zstd::libzstd_shared INCLUDE_DIRECTORIES) 
#message(STATUS "zstd::libzstd_shared works!")

# Find or fetch ZSTD
include(FetchContent)

set(TOOLS_DEP_LIBS)
message(STATUS "fetching ZSTD")
FetchContent_Declare(
	ZSTD
	GIT_REPOSITORY 	"https://github.com/facebook/zstd.git"
	GIT_TAG			v1.5.5
	# URL "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz"
	SOURCE_SUBDIR build/cmake
	OVERRIDE_FIND_PACKAGE
	)


find_package(zstd REQUIRED)
add_library(zstd::libzstd_shared ALIAS libzstd_shared)
add_library(zstd::libzstd_static ALIAS libzstd_static)

set(ZSTD_INCLUDE_DIR ${zstd_SOURCE_DIR}/lib)

list(APPEND TOOLS_DEP_LIBS zstd::libzstd_static)


 
add_definitions("-DBUILD_TOOLS_LIB")
SET(LIBRIR_TOOLS_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/FileAttributes.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/FileLock.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Log.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Misc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ReadFileChunk.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/SIMD.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tools.cpp

	# ${ZSTD_SRC}
	${BLOSC_SRC}
	${MINIZIP_SRC}
	)

SET(LIBRIR_TOOLS_SRC ${LIBRIR_TOOLS_SRC} PARENT_SCOPE)

set(TOOLS_HEADERS 
	${CMAKE_CURRENT_SOURCE_DIR}/SIMD.h
	${CMAKE_CURRENT_SOURCE_DIR}/FileAttributes.h
	${CMAKE_CURRENT_SOURCE_DIR}/FileLock.h
	${CMAKE_CURRENT_SOURCE_DIR}/Log.h
	${CMAKE_CURRENT_SOURCE_DIR}/Misc.h
	${CMAKE_CURRENT_SOURCE_DIR}/ReadFileChunk.h
	${CMAKE_CURRENT_SOURCE_DIR}/tools.h
)

add_library(tools ${LIBRIR_TOOLS_SRC})

if (NOT WIN32)
	list(APPEND TOOLS_DEP_LIBS pthread)
endif()
	
target_link_libraries(tools PRIVATE ${TOOLS_DEP_LIBS})

target_include_directories(tools PRIVATE ${ZSTD_INCLUDE_DIR})

# target_include_directories(tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(	tools 
							INTERFACE 	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
										$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)


target_include_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY})
target_include_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/blosc)
target_include_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/minizip)

MESSAGE(STATUS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set_target_properties(tools PROPERTIES
	PUBLIC_HEADER "${TOOLS_HEADERS}" 
	# INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR};${CMAKE_CURRENT_SOURCE_DIR};${EXTRAS_INCLUDE_DIRECTORY}" 
	# LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	# ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	# RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	# RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	# RUNTIME_OUTPUT_DIRECTORY ${DESTINATION_DIR}
	)

get_target_property(TOOLS_INCLUDE_DIRS tools INCLUDE_DIRECTORIES)



