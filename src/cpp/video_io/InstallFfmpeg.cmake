


MESSAGE(STATUS "Ffmpeg libs path:  ${COMPLETE_FFMPEG_LIBS}")

foreach(ffmpeg_lib IN ITEMS ${COMPLETE_FFMPEG_LIBS} )
	MESSAGE(STATUS "CORRECT_FFMPEG_LIB_PATH: ${CORRECT_FFMPEG_LIB_PATH}")

	file(GLOB CORRECT_FFMPEG_LIB_PATH "${FFMPEG_LIB_DIR}/*${ffmpeg_lib}*${FFMPEG_SUFFIX}*")
	foreach(ffmpeg_lib_path ${CORRECT_FFMPEG_LIB_PATH})
		
		IF(ffmpeg_lib_path)
			MESSAGE(STATUS "Found file ${ffmpeg_lib_path}, copy to ${FFMPEG_INSTALL_DIR}")
			file(COPY ${ffmpeg_lib_path} DESTINATION ${FFMPEG_INSTALL_DIR})
		endif()
	endforeach()
endforeach()

IF (WIN32)
	IF (NOT MSVC)
		MESSAGE("Custom mingw installation")
		# mingw/msys2
		file(GLOB ffmpeg_dlls "${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg/install/bin/*.dll")
		MESSAGE("Copy files: ${ffmpeg_dlls}")
		file(COPY ${ffmpeg_dlls} DESTINATION ${FFMPEG_INSTALL_DIR})
	ENDIF()
ENDIF()