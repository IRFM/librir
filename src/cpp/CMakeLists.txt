SET(TARGETS tools geometry signal_processing video_io)

add_subdirectory(tools)
add_subdirectory(geometry)
add_subdirectory(signal_processing)
add_subdirectory(video_io)

IF (NOT MSVC)
    # some dependency to prevent from making librir before building ffmpeg
    add_dependencies(video_io dummy)
ENDIF()

install(
    TARGETS ${TARGETS} 
    PUBLIC_HEADER
    ARCHIVE
    RUNTIME
    )
install(
    TARGETS ${TARGETS} 
    EXPORT LibrirTargets
    )
install(EXPORT LibrirTargets
        FILE LibrirTargets.cmake 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake/librir
        NAMESPACE librir::
)
set(LIBRIR_TARGETS ${TARGETS} PARENT_SCOPE)


