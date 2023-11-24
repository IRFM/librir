SET(TARGETS tools geometry signal_processing video_io)
# MESSAGE(STATUS "TARGETS : " ${TARGETS})
add_subdirectory(tools)
add_subdirectory(geometry)
add_subdirectory(signal_processing)
add_subdirectory(video_io)

IF (NOT MSVC)
    # some dependency to prevent from making librir before building ffmpeg
    add_dependencies(video_io dummy)
ENDIF()

install (TARGETS ${TARGETS} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
install (TARGETS ${TARGETS} LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR} )
install(
    TARGETS ${TARGETS} 
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


# message(STATUS ${EXCLUDED_TARGETS})


