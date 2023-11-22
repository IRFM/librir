include(FetchContent)


# SET(ZSTD_DIR ${CMAKE_SOURCE_DIR}/extra/zstd)
# SET(ZSTD_SRC ${ZSTD_DIR}/extra/zstd)
message(STATUS "fetching ZSTD")

find_package(zstd)


if(NOT zstd_FOUND)
	FetchContent_Declare(
		zstd
		GIT_REPOSITORY	https://github.com/facebook/zstd.git
		)
		FetchContent_MakeAvailable(zstd)
	if(zstd_POPULATED)
		find_package(zstd)
		
		# set(ZSTD_INCLUDE_DIR ${zstd_SOURCE_DIR}/../../lib)

		# add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})
		# include_directories(${zstd_SOURCE_DIR}/../../lib)
		# link_directories(${libzstd_BINARY_DIR}/${CMAKE_BUILD_TYPE_INIT})
		# include_directories(${zstd_SOURCE_DIR}/lib)

	endif()	

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
if (NOT WIN32)
	target_link_libraries(tools pthread)
# else()
# 	target_link_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/blosc/win32)
# 	target_include_directories(tools PRIVATE ${EXTRAS_INCLUDE_DIRECTORY}/blosc/win32)

endif()
# MESSAGE(STATUS "EXTRAS_INCLUDE_DIRECTORY : " ${EXTRAS_INCLUDE_DIRECTORY})
target_link_libraries(tools zstd::libzstd_shared)
get_target_property(ZSTD_INCLUDE_DIRECTORIES zstd::libzstd_shared 
  INTERFACE_INCLUDE_DIRECTORIES 
)
target_include_directories(tools INTERFACE zstd::libzstd_shared::INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(tools PUBLIC ${zstd_SOURCE_DIR}/lib)

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
