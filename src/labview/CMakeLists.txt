# TODO: some packaging to make it installable



install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
	DESTINATION labview
	FILES_MATCHING PATTERN "*"
)


install(IMPORTED_RUNTIME_ARTIFACTS ${LIBRIR_TARGETS} ${FFMPEG_SHARED_LIBRARIES} ${FFMPEG_ADDITIONAL_DEPENDENCIES}
		# RUNTIME_DEPENDENCY_SET labview-set
		# RUNTIME
		DESTINATION labview/libs
)


# for development purposes
install(IMPORTED_RUNTIME_ARTIFACTS ${LIBRIR_TARGETS} 
		RUNTIME_DEPENDENCY_SET labview-dev-set
		RUNTIME
		DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/libs
)

set(_ffmpeg_commands "")

list(APPEND _ALL_FFMPEG_SHARED_LIBS ${FFMPEG_SHARED_LIBRARIES})
list(APPEND _ALL_FFMPEG_SHARED_LIBS ${FFMPEG_ADDITIONAL_DEPENDENCIES})
list(REMOVE_DUPLICATES _ALL_FFMPEG_SHARED_LIBS)

foreach(_ffmpeg_shared_lib_target IN LISTS _ALL_FFMPEG_SHARED_LIBS)
	get_target_property(_ffmpeg_shared_lib ${_ffmpeg_shared_lib_target} IMPORTED_LOCATION)
	set_target_properties(${_ffmpeg_shared_lib_target} PROPERTIES LINKER_LANGUAGE C)

	list(APPEND _ffmpeg_commands "$<TARGET_FILE:${_ffmpeg_shared_lib_target}>")
endforeach()

add_custom_target(copy_dlls_to_labview_package_librir ALL
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/libs
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	$<TARGET_FILE:video_io> $<TARGET_RUNTIME_DLLS:video_io>
	$<TARGET_FILE:tools> $<TARGET_RUNTIME_DLLS:tools>
	$<TARGET_FILE:geometry> $<TARGET_RUNTIME_DLLS:geometry>
	$<TARGET_FILE:signal_processing> $<TARGET_RUNTIME_DLLS:signal_processing>
	${_ffmpeg_commands}
	${CMAKE_CURRENT_SOURCE_DIR}/libs
	DEPENDS video_io
	COMMAND_EXPAND_LISTS
)