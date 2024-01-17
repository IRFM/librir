
add_subdirectory(cpp)
add_subdirectory(python)
add_subdirectory(labview)
set(LIBRIR_TARGETS ${LIBRIR_TARGETS} PARENT_SCOPE)
install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/
		DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/labview/libs
)