
set(TARGET_NAME librir)

add_custom_target(${TARGET_NAME})

add_dependencies(${TARGET_NAME} tools geometry signal_processing video_io west)


install(
	CODE
		"
		file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/librir/libs)

		IF(MSVC)
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			   COMMAND ${CMAKE_COMMAND} -E copy_if_different
				   ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/*.dll ${CMAKE_INSTALL_PREFIX}/librir/libs)
		ELSE()
			add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy
					   ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/* ${CMAKE_INSTALL_PREFIX}/librir/libs)
		ENDIF()
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
			   COMMAND ${CMAKE_COMMAND} -E copy_if_different
				   ${CMAKE_SOURCE_DIR}/setup.py ${CMAKE_INSTALL_PREFIX})
		   
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
		   COMMAND ${CMAKE_COMMAND} -E copy_if_different
			   ${CMAKE_SOURCE_DIR}/CMakeLists.txt ${CMAKE_INSTALL_PREFIX})
		   
		add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
		   COMMAND ${CMAKE_COMMAND} -E copy_if_different
			   ${CMAKE_SOURCE_DIR}/extra-requirements.txt ${CMAKE_INSTALL_PREFIX})"

)