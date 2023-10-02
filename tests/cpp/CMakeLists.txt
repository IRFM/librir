

# add_definitions("-DTSLIB_COMPRESS")

# AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/tslib TSLIB_SRC)




# get_target_property(TOOLS_HEADERS tools PUBLIC_HEADER)
# get_target_property(TOOLS_HEADERS_DIR tools INCLUDE_DIRECTORIES)

# get_target_property(GEOMETRY_HEADERS geometry PUBLIC_HEADER)
# get_target_property(GEOMETRY_HEADERS_DIR geometry INCLUDE_DIRECTORIES)

# get_target_property(SIGNAL_PROCESSING_HEADERS signal_processing PUBLIC_HEADER)
# get_target_property(SIGNAL_PROCESSING_HEADERS_DIR signal_processing INCLUDE_DIRECTORIES)


# get_target_property(VIDEO_IO_HEADERS video_io PUBLIC_HEADER)
# get_target_property(VIDEO_IO_HEADERS_DIR video_io INCLUDE_DIRECTORIES)

# get_target_property(WEST_HEADERS west PUBLIC_HEADER)
# get_target_property(WEST_HEADERS_DIR west INCLUDE_DIRECTORIES)


# SET(LIBRIR_TEST_SRC
	# ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
	# )

# #~ include_directories(${CMAKE_CURRENT_SOURCE_DIR}
	# #~ ${TOOLS_HEADERS_DIR}
	# #~ ${GEOMETRY_HEADERS_DIR}
	# #~ ${SIGNAL_PROCESSING_HEADERS_DIR}
	# #~ ${VIDEO_IO_HEADERS_DIR}
	# #~ ${WEST_HEADERS_DIR}
	# #~ ${ZSTD_INCLUDE_DIR}
	# #~ )


# # include_directories(${ZSTD_INCLUDE_DIR})

# if (MSVC)
	# SET(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg-4.3-msvc/lib)
	# SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter Vfw32 shlwapi strmbase)
# ELSE()
	# SET(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg/install/lib)
	# SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
# ENDIF()
# link_directories(${FFMPEG_LIB_DIR})


# SET(LIBRIR_TEST_SRC ${LIBRIR_TEST_SRC} PARENT_SCOPE)
# # SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter Vfw32 shlwapi strmbase)
# # set(WEST_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/west.h")

# add_executable(test_west ${LIBRIR_TEST_SRC})
# add_executable(test_video_io ${LIBRIR_TEST_SRC})
# add_dependencies(test_west west)
# target_link_libraries(test_west west video_io tools geometry signal_processing)# ${FFMPEG_LIBS})
# target_include_directories(test_west PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# set_target_properties(test_west PROPERTIES
	# PUBLIC_HEADER "${TEST_HEADERS}"
	# # LIBRARY_OUTPUT_DIRECTORY ${DESTINATION_DIR}
	# # RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
# )

# add_test(NAME test_west 
		 # WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/out
		 # COMMAND test_west)



