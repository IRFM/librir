
if (MSVC)
	SET(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg-4.3-msvc/lib)
	SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter Vfw32 shlwapi strmbase)
ELSE()
	SET(FFMPEG_LIB_DIR ${CMAKE_SOURCE_DIR}/3rd_64/ffmpeg/install/lib)
	SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
ENDIF()
link_directories(${FFMPEG_LIB_DIR})


# create the testing file and list of tests
create_test_sourcelist (Tests_WEST
	CommonCxxTests.cxx
	test_call_ts.cpp
	test_full.cpp
	# test_HCC.cpp
  )

# add the executable
add_executable (CommonCxxTests ${Tests_WEST})

# remove the test driver source file
set (TestsToRun ${Tests_WEST})
remove (TestsToRun CommonCxxTests.cxx)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
  get_filename_component (TName ${test} NAME_WE)
  add_test (NAME ${TName} COMMAND CommonCxxTests ${TName})
endforeach ()

# add_executable(test_call_ts test_call_ts.cpp)
add_dependencies(CommonCxxTests west)


target_include_directories(CommonCxxTests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(CommonCxxTests  video_io tools geometry signal_processing west)# ${FFMPEG_LIBS})

# add_test(NAME test_call_ts 
		 # WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/out
		 # COMMAND test_call_ts)