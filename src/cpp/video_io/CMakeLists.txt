
add_definitions("-DBUILD_IO_LIB")

# get previous targets include folders
get_target_property(TOOLS_HEADERS tools PUBLIC_HEADER)
get_target_property(TOOLS_HEADERS_DIR tools INCLUDE_DIRECTORIES)

get_target_property(GEOMETRY_HEADERS geometry PUBLIC_HEADER)
get_target_property(GEOMETRY_HEADERS_DIR geometry INCLUDE_DIRECTORIES)

get_target_property(SIGNAL_PROCESSING_HEADERS signal_processing PUBLIC_HEADER)
get_target_property(SIGNAL_PROCESSING_HEADERS_DIR signal_processing INCLUDE_DIRECTORIES)


# Set sources

SET(LIBRIR_IO_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/BaseCalibration.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/IRFileLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/h264.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/IRVideoLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/HCCLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ZFile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/video_io.cpp
	# ${CMAKE_CURRENT_SOURCE_DIR}/ZFile.cpp
	)

SET(LIBRIR_IO_SRC ${LIBRIR_IO_SRC} PARENT_SCOPE)

set(IO_HEADERS 
${CMAKE_CURRENT_SOURCE_DIR}/IRVideoLoader.h
${CMAKE_CURRENT_SOURCE_DIR}/IRFileLoader.h
${CMAKE_CURRENT_SOURCE_DIR}/BaseCalibration.h
${CMAKE_CURRENT_SOURCE_DIR}/HCCLoader.h
${CMAKE_CURRENT_SOURCE_DIR}/ZFile.h
${CMAKE_CURRENT_SOURCE_DIR}/video_io.h
${CMAKE_CURRENT_SOURCE_DIR}/h264.h
# ${CMAKE_CURRENT_SOURCE_DIR}/ZFile.h
)

# set library video_io

add_library(video_io ${LIBRIR_IO_SRC})

# retrieve ffmpeg dependency manually since there is no easy way 
# to fetch and compile sources for windows and linux
# FIXME: FetchContent/find_package ffmpeg
IF (MSVC)
	SET(FFMPEG_LIB_DIR ${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg-4.3-msvc/lib)
	SET(FFMPEG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg-4.3-msvc)
	SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter )
	SET(ADDITIONAL_FFMPEG_LIBS libx264 libx265 kvazaar postproc vcruntime140_1 )
	SET(FFMPEG_SYSTEM_DEPENDENCIES Vfw32 shlwapi strmbase)
	SET(FFMPEG_PREFIX )
	SET(FFMPEG_SUFFIX .dll)
	SET(FFMPEG_IMPLIB .lib)
	
ELSE()
	IF (WIN32)
		# mingw
		SET(FFMPEG_LIB_DIR ${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg/install/lib)
		SET(FFMPEG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg/install/include)
		SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
		SET(ADDITIONAL_FFMPEG_LIBS postproc)
		SET(FFMPEG_SYSTEM_DEPENDENCIES )

		SET(FFMPEG_PREFIX )
		SET(FFMPEG_SUFFIX .dll)
		SET(FFMPEG_IMPLIB .lib)

	ELSE()
		ExternalProject_Get_property(ffmpeg INSTALL_DIR)
		SET(FFMPEG_LIB_DIR ${INSTALL_DIR}/lib)
		SET(FFMPEG_INCLUDE_DIR ${INSTALL_DIR}/include)
		SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
		SET(ADDITIONAL_FFMPEG_LIBS postproc)
		SET(FFMPEG_SYSTEM_DEPENDENCIES )
		SET(FFMPEG_PREFIX lib)
		SET(FFMPEG_SUFFIX .so)
		SET(FFMPEG_IMPLIB .a)

	ENDIF()
ENDIF()

MESSAGE(STATUS "Using ${FFMPEG_LIB_DIR} for ffmpeg library files")
set(COMPLETE_FFMPEG_LIBS ${FFMPEG_LIBS} ${ADDITIONAL_FFMPEG_LIBS})
set(FFMPEG_DLLS_DEPEDENCIES "" CACHE INTERNAL "")

if (WIN32 OR MSVC)  
	foreach(ffmpeg_lib ${FFMPEG_LIBS})
		file(GLOB CORRECT_FFMPEG_SHARED_LIB_PATH_LIST "${FFMPEG_LIB_DIR}/*${ffmpeg_lib}*${FFMPEG_SUFFIX}*")
		list(GET CORRECT_FFMPEG_SHARED_LIB_PATH_LIST 0 CORRECT_FFMPEG_SHARED_LIB_PATH)
		file(GLOB CORRECT_FFMPEG_IMPLIB_PATH_LIST "${FFMPEG_LIB_DIR}/*${ffmpeg_lib}*${FFMPEG_IMPLIB}*")
		list(GET CORRECT_FFMPEG_IMPLIB_PATH_LIST 0 CORRECT_FFMPEG_IMPLIB_PATH)
		add_library(${ffmpeg_lib} SHARED IMPORTED)
		set_target_properties(${ffmpeg_lib} PROPERTIES
			IMPORTED_LOCATION ${CORRECT_FFMPEG_SHARED_LIB_PATH}
			
		)

		get_filename_component(__dll_name ${CORRECT_FFMPEG_SHARED_LIB_PATH} NAME)
		set_target_properties(${ffmpeg_lib} PROPERTIES
			RUNTIME_OUTPUT_NAME ${__dll_name}
		)
		
		set_target_properties(${ffmpeg_lib} PROPERTIES
		IMPORTED_IMPLIB ${CORRECT_FFMPEG_IMPLIB_PATH}
		)
		set_target_properties(${ffmpeg_lib} PROPERTIES
			INCLUDE_DIRECTORIES ${FFMPEG_INCLUDE_DIR}/lib${ffmpeg_lib}
		)
		list(APPEND FFMPEG_DLLS_DEPEDENCIES ${CORRECT_FFMPEG_SHARED_LIB_PATH})

	endforeach()
	foreach(_additional_ffmpeg_lib ${ADDITIONAL_FFMPEG_LIBS})
		file(GLOB CORRECT_FFMPEG_SHARED_LIB_PATH "${FFMPEG_LIB_DIR}/*${_additional_ffmpeg_lib}*${FFMPEG_SUFFIX}*")
		list(APPEND FFMPEG_DLLS_DEPEDENCIES ${CORRECT_FFMPEG_SHARED_LIB_PATH})
	endforeach()
endif()

set(FFMPEG_DLLS_DEPEDENCIES ${FFMPEG_DLLS_DEPEDENCIES} CACHE INTERNAL "")
set(FFMPEG_LIB_DIR ${FFMPEG_LIB_DIR} CACHE INTERNAL "")
set(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR} CACHE INTERNAL "")
set(FFMPEG_LIBS ${FFMPEG_LIBS} CACHE INTERNAL "")
set(ADDITIONAL_FFMPEG_LIBS ${ADDITIONAL_FFMPEG_LIBS} CACHE INTERNAL "")
set(FFMPEG_PREFIX ${FFMPEG_PREFIX} CACHE INTERNAL "")
set(FFMPEG_SUFFIX ${FFMPEG_SUFFIX} CACHE INTERNAL "")
set(COMPLETE_FFMPEG_LIBS ${COMPLETE_FFMPEG_LIBS} CACHE INTERNAL "")

# include and linking
target_link_libraries(video_io PUBLIC tools geometry signal_processing)

target_include_directories(	video_io 
							INTERFACE 	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
										$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
							PUBLIC 		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> 
										$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(	video_io 
						PRIVATE 	${FFMPEG_LIBS}
								
)

target_link_directories(video_io 
						PRIVATE 	$<BUILD_INTERFACE:${FFMPEG_LIB_DIR}>
									$<INSTALL_INTERFACE:${CMAKE_INSTALL_LIBDIR}>
)
target_include_directories(video_io PRIVATE ${FFMPEG_INCLUDE_DIR})


set_target_properties(video_io PROPERTIES
	PUBLIC_HEADER "${IO_HEADERS}"
)
get_target_property(IO_RUNTIME_OUTPUT_DIRECTORY video_io RUNTIME_OUTPUT_DIRECTORY)

MESSAGE(STATUS "IO_RUNTIME_OUTPUT_DIRECTORY : ${IO_RUNTIME_OUTPUT_DIRECTORY}")

# installing FFMPEG dependencies 
install(CODE "set(FFMPEG_INSTALL_DIR \"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\")")
install(SCRIPT InstallFfmpeg.cmake)
