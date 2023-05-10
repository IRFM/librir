IF(Python_FOUND)

	add_test (NAME python-tests-fast 
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		COMMAND ${Python_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR} -m "not slow"
	)

endif()