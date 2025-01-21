SET(TARGETS tools geometry signal_processing video_io)

add_subdirectory(tools)
add_subdirectory(geometry)
add_subdirectory(signal_processing)
add_subdirectory(video_io)

IF (NOT MSVC)
	if (BUILD_FFMPEG)
		# some dependency to prevent from making librir before building ffmpeg
		add_dependencies(video_io ffmpeg)
	endif()
ENDIF()


set(LIBRIR_TARGETS ${TARGETS} CACHE INTERNAL "")


