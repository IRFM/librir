option(BUILD_WHEEL "Building python wheels during install" OFF)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/librir
    DESTINATION python
    FILES_MATCHING PATTERN "*.py"
)

install(DIRECTORY $${CMAKE_CURRENT_SOURCE_DIR}/librir/LICENSES
    DESTINATION python/
    FILES_MATCHING PATTERN "*.txt"
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/librir/LICENSE
    DESTINATION python/librir
)

LIST(APPEND pre_exclude_regexes "api-ms-.*")
LIST(APPEND pre_exclude_regexes "ext-ms-.*")
LIST(APPEND post_exclude_regexes ".*WINDOWS[\\/]system32.*")
LIST(APPEND post_exclude_regexes "^/lib" "^/usr" "^/bin")

install(IMPORTED_RUNTIME_ARTIFACTS ${LIBRIR_TARGETS} ${FFMPEG_SHARED_LIBRARIES} ${FFMPEG_ADDITIONAL_DEPENDENCIES}

    # RUNTIME_DEPENDENCY_SET ffmpeg-set
    # COMPONENT Python
    # PRE_EXCLUDE_REGEXES ${pre_exclude_regexes}
    # POST_EXCLUDE_REGEXES ${post_exclude_regexes}
    DESTINATION python/librir/libs
)

# install(IMPORTED_RUNTIME_ARTIFACTS ${FFMPEG_SHARED_LIBRARIES} ${FFMPEG_ADDITIONAL_DEPENDENCIES}

# DESTINATION ${CMAKE_INSTALL_LIBDIR})
configure_file(pyproject.toml.in pyproject.toml @ONLY)
configure_file(pyproject.toml.in ${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pyproject.toml ${PROJECT_SOURCE_DIR}/README.md
    DESTINATION python
)

# setting version in __init__.py
function(cat IN_FILE OUT_FILE)
    file(READ ${IN_FILE} CONTENTS)
    file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

set(INIT_PY_FILES)
file(REMOVE __init__.py.in)
file(WRITE __init__.py.in "")

cat(${CMAKE_CURRENT_SOURCE_DIR}/librir/__init__.py __init__.py.in)
file(APPEND __init__.py.in "\n__version__ = \"@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@\"")

configure_file(__init__.py.in __init__.py @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/__init__.py DESTINATION python/librir/)
FILE(REMOVE __init__.py.in)

# for development purposes
install(IMPORTED_RUNTIME_ARTIFACTS ${LIBRIR_TARGETS} ${FFMPEG_SHARED_LIBRARIES} ${FFMPEG_ADDITIONAL_DEPENDENCIES}
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/librir/libs
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pyproject.toml ${PROJECT_SOURCE_DIR}/README.md
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
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

add_custom_target(copy_dlls_to_python_package_librir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/librir/libs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:video_io> $<TARGET_RUNTIME_DLLS:video_io>
    $<TARGET_FILE:tools> $<TARGET_RUNTIME_DLLS:tools>
    $<TARGET_FILE:geometry> $<TARGET_RUNTIME_DLLS:geometry>
    $<TARGET_FILE:signal_processing> $<TARGET_RUNTIME_DLLS:signal_processing>
    ${_ffmpeg_commands}
    ${CMAKE_CURRENT_SOURCE_DIR}/librir/libs
    DEPENDS video_io
    COMMAND_EXPAND_LISTS
)

# message(STATUS "_ALL_FFMPEG_SHARED_LIBS : ${_ALL_FFMPEG_SHARED_LIBS}")

# foreach(_ffmpeg_shared_lib_target IN LISTS _ALL_FFMPEG_SHARED_LIBS)
# get_target_property(_ffmpeg_shared_lib ${_ffmpeg_shared_lib_target} IMPORTED_LOCATION)
# message(STATUS "_ffmpeg_shared_lib : ${_ffmpeg_shared_lib}")

# string(REPLACE "FFMPEG::" "" _ffmpeg_component ${_ffmpeg_shared_lib_target})
# message(STATUS "_ffmpeg_component : ${_ffmpeg_component}")
# message(STATUS "AAAAAAAAAA : ${CMAKE_CURRENT_SOURCE_DIR}/librir/libs")

# add_custom_command(OUTPUT custom_${_ffmpeg_component}
# COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${_ffmpeg_shared_lib_target}> $<TARGET_RUNTIME_DLLS:${_ffmpeg_shared_lib_target}> ${CMAKE_CURRENT_SOURCE_DIR}/librir/libs/
# MAIN_DEPENDENCY $<TARGET_FILE:video_io>
# )
# endforeach()
find_program(PIP_EXECUTABLE "pip" HINTS ENV "PATH" PATHS ENV PIP_EXECUTABLE)

if(PIP_EXECUTABLE AND BUILD_WHEEL)
    install(CODE "execute_process(COMMAND ${PIP_EXECUTABLE} wheel ${CMAKE_CURRENT_SOURCE_DIR} -w ${CMAKE_INSTALL_PREFIX}/dist --no-deps)")
endif()

# On Linux plateforms:
# Copy west shared libs to python/librir_west/libs with links, at it is not properly done by cmake install
if(UNIX AND NOT APPLE)
	install(CODE "execute_process(COMMAND cp --verbose -a ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libtools.so.${PROJECT_VERSION} ${CMAKE_INSTALL_PREFIX}/python/librir/libs/)")
	install(CODE "execute_process(COMMAND cp --verbose -a ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libgeometry.so.${PROJECT_VERSION} ${CMAKE_INSTALL_PREFIX}/python/librir/libs/)")
	install(CODE "execute_process(COMMAND cp --verbose -a ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libsignal_processing.so.${PROJECT_VERSION} ${CMAKE_INSTALL_PREFIX}/python/librir/libs/)")
	install(CODE "execute_process(COMMAND cp --verbose -a ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libvideo_io.so.${PROJECT_VERSION} ${CMAKE_INSTALL_PREFIX}/python/librir/libs/)")
endif()