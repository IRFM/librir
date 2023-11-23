include(FetchContent)


# SET(ZSTD_DIR ${CMAKE_SOURCE_DIR}/extra/zstd)
# SET(ZSTD_SRC ${ZSTD_DIR}/extra/zstd)
message(STATUS "fetching ZSTD")

find_package(zstd)
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED OFF)

if(NOT ZSTD_FOUND)


	# FetchContent_Declare(
	# 	zstd
	# 	GIT_REPOSITORY	https://github.com/facebook/zstd.git
	# 	)

	FetchContent_Declare(
		zstd
		URL "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz"
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
		SOURCE_SUBDIR build/cmake
	)

	FetchContent_MakeAvailable(zstd)

	# if(zstd_POPULATED)
	# 	add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})
	# 	set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${zstd_SOURCE_DIR}/build/cmake)
		
	# 	# set(ZSTD_INCLUDE_DIR ${zstd_SOURCE_DIR}/../../lib)

	# 	# include_directories(${zstd_SOURCE_DIR}/../../lib)
	# 	# link_directories(${libzstd_BINARY_DIR}/${CMAKE_BUILD_TYPE_INIT})
	# 	# include_directories(${zstd_SOURCE_DIR}/lib)

	# endif()	
	# find_package(zstd)

endif()

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

#~ include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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

set(TOOLS_DEP_LIBS libzstd_static)
if (NOT WIN32)
	set(TOOLS_DEP_LIBS ${TOOLS_DEP_LIBS} pthread)
endif()
# MESSAGE(STATUS "EXTRAS_INCLUDE_DIRECTORY : " ${EXTRAS_INCLUDE_DIRECTORY})
	
target_link_libraries(tools PRIVATE ${TOOLS_DEP_LIBS})

target_include_directories(tools PRIVATE ${zstd_SOURCE_DIR}/lib)
MESSAGE(STATUS ${zstd_SOURCE_DIR}/lib)
target_include_directories(tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY})
target_include_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/blosc)
# target_include_directories(tools PRIVATE ${ZSTD_INCLUDE_DIR})

target_include_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/minizip)

# target_link_directories(tools PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY})
# target_link_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/blosc)
# target_link_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/zstd)
# target_link_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/minizip)

MESSAGE(STATUS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set_target_properties(tools PROPERTIES
	PUBLIC_HEADER "${TOOLS_HEADERS}"
	# LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	# ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	# RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	# RUNTIME_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
	# RUNTIME_OUTPUT_DIRECTORY ${DESTINATION_DIR}
)

set(ZSTD_INCLUDE_DIR ${ZSTD_INCLUDE_DIR} PARENT_SCOPE)
