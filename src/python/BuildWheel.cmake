
execute_process(
	COMMAND pip wheel ${CMAKE_INSTALL_PREFIX}/python -w ${CMAKE_CURRENT_BINARY_DIR}/dist --no-deps
	# commands
)


file(GLOB wheel_files ${CMAKE_CURRENT_BINARY_DIR}/dist/*.whl )
file(COPY ${wheel_files} DESTINATION ${CMAKE_INSTALL_PREFIX}/dist )

# file(
#     COPY ${CMAKE_CURRENT_BINARY_DIR}/dist
# 	DESTINATION ${CMAKE_INSTALL_PREFIX}
# )