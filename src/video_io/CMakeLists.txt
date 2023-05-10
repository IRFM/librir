
add_definitions("-DBUILD_IO_LIB")


get_target_property(TOOLS_HEADERS tools PUBLIC_HEADER)
get_target_property(TOOLS_HEADERS_DIR tools INCLUDE_DIRECTORIES)

get_target_property(GEOMETRY_HEADERS geometry PUBLIC_HEADER)
get_target_property(GEOMETRY_HEADERS_DIR geometry INCLUDE_DIRECTORIES)

get_target_property(SIGNAL_PROCESSING_HEADERS signal_processing PUBLIC_HEADER)
get_target_property(SIGNAL_PROCESSING_HEADERS_DIR signal_processing INCLUDE_DIRECTORIES)

# find_package(FFmpeg COMPONENTS AVDEVICE REQUIRED)


SET(LIBRIR_IO_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/BaseCalibration.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/IRFileLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/h264.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/IRVideoLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/HCCLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/video_io.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ZFile.cpp
	)

IF (MSVC)
	SET(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg-4.3-msvc/lib)
	SET(FFMPEG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg-4.3-msvc)
	SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter )
	SET(ADDITIONAL_FFMPEG_LIBS libx264 libx265 kvazaar postproc vcruntime140_1 Vfw32 shlwapi strmbase)
	SET(FFMPEG_PREFIX )
	SET(FFMPEG_SUFFIX .dll)
	
ELSE()

	IF (WIN32)
		# mingw
		SET(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg/install/lib)
		SET(FFMPEG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg/install/include)
		SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
		SET(ADDITIONAL_FFMPEG_LIBS postproc)
		SET(FFMPEG_PREFIX )
		SET(FFMPEG_SUFFIX .dll)
	ELSE()
		SET(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg/install/lib)
		SET(FFMPEG_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg/install/include)
		SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
		SET(ADDITIONAL_FFMPEG_LIBS postproc)
		SET(FFMPEG_PREFIX lib)
		SET(FFMPEG_SUFFIX .so)
	ENDIF()
ENDIF()
link_directories(${FFMPEG_LIB_DIR})
MESSAGE(STATUS "Using ${FFMPEG_LIB_DIR} for ffmpeg library files")


SET(LIBRIR_IO_SRC ${LIBRIR_IO_SRC} PARENT_SCOPE)

set(IO_HEADERS 
	${CMAKE_CURRENT_SOURCE_DIR}/IRVideoLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/IRFileLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/BaseCalibration.h
	${CMAKE_CURRENT_SOURCE_DIR}/HCCLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/video_io.h
	${CMAKE_CURRENT_SOURCE_DIR}/h264.h
	${CMAKE_CURRENT_SOURCE_DIR}/ZFile.h
)

add_library(video_io ${LIBRIR_IO_SRC})
add_dependencies(video_io tools geometry signal_processing)
target_link_libraries(video_io PRIVATE tools geometry signal_processing ${FFMPEG_LIBS})
target_include_directories(video_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories(video_io PRIVATE 	${FFMPEG_INCLUDE_DIR})



set_target_properties(video_io PROPERTIES
	PUBLIC_HEADER "${IO_HEADERS}"
	#~ LIBRARY_OUTPUT_DIRECTORY ${DESTINATION_DIR}
	#~ RUNTIME_OUTPUT_DIRECTORY ${DESTINATION_DIR}
)
get_target_property(IO_RUNTIME_OUTPUT_DIRECTORY video_io RUNTIME_OUTPUT_DIRECTORY)

MESSAGE(STATUS "${IO_RUNTIME_OUTPUT_DIRECTORY}")
SET(COMPLETE_FFMPEG_LIBS ${FFMPEG_LIBS} ${ADDITIONAL_FFMPEG_LIBS})

file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

# Add any variables you need during post install.
install(CODE "set(FFMPEG_LIB_DIR \"${FFMPEG_LIB_DIR}\")")
install(CODE "set(FFMPEG_INCLUDE_DIR \"${FFMPEG_INCLUDE_DIR}\")")
install(CODE "set(FFMPEG_LIBS \"${FFMPEG_LIBS}\")")
install(CODE "set(ADDITIONAL_FFMPEG_LIBS \"${ADDITIONAL_FFMPEG_LIBS}\")")
install(CODE "set(FFMPEG_PREFIX \"${FFMPEG_PREFIX}\")")
install(CODE "set(FFMPEG_SUFFIX \"${FFMPEG_SUFFIX}\")")
install(CODE "set(COMPLETE_FFMPEG_LIBS \"${COMPLETE_FFMPEG_LIBS}\")")
install(CODE "set(CMAKE_INSTALL_BINDIR \"${CMAKE_INSTALL_BINDIR}\")")
install(CODE "set(CMAKE_SOURCE_DIR \"${CMAKE_SOURCE_DIR}\")")
install(SCRIPT InstallFfmpeg.cmake)
# foreach(ffmpeg_lib IN ITEMS ${COMPLETE_FFMPEG_LIBS} )
	# file(GLOB CORRECT_FFMPEG_LIB_PATH "${FFMPEG_LIB_DIR}/*${ffmpeg_lib}*${FFMPEG_SUFFIX}*")
	# foreach(ffmpeg_lib_path ${CORRECT_FFMPEG_LIB_PATH})
		
		# IF(ffmpeg_lib_path)
			# install(FILES ${ffmpeg_lib_path} DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
		# endif()
	# endforeach()
# endforeach()

# IF (WIN32)
	# IF (NOT MSVC)
		# # mingw/msys2
		# file(GLOB ffmpeg_dlls "${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg/install/bin/*.dll")
		# install(FILES ${ffmpeg_dlls} DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR})
	# ENDIF()
# ENDIF()
