



# set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/Users/LD243615/Anaconda3/Lib/site-packages/pybind11/share/cmake/pybind11")
FetchContent_Declare(
	pybind11
	GIT_REPOSITORY
	https://github.com/pybind/pybind11.git
	GIT_TAG         master
	OVERRIDE_FIND_PACKAGE 
)
find_package(pybind11 CONFIG REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})

message([MAIN] "Found pybind11 v${pybind11_VERSION}: ${pybind11_INCLUDE_DIRS}")

MESSAGE( [Main] " pybind11_INCLUDE_DIRS = ${pybind11_INCLUDE_DIRS}")
MESSAGE( [Main] " pybind11_LIBRARIES = ${pybind11_LIBRARIES}")

# pybind11_add_module(westbind pybind11_wrapper.cpp)
set(__python_bindings)
foreach(target ${LIBRIR_TARGETS})
    set(_pythonbind_target_name rir_${target})
    add_library(${_pythonbind_target_name} MODULE ${target}_pybind11_wrapper.cpp)
    SET_TARGET_PROPERTIES( ${_pythonbind_target_name}
        PROPERTIES
        # OUTPUT_NAME ${target}
        SUFFIX ".pyd"
        INSTALL_RPATH "@loader_path/../libs"
        )
    get_target_property(__INCLUDE ${target} INCLUDE_DIRECTORIES)
    target_include_directories(${_pythonbind_target_name} PUBLIC ${__INCLUDE})
    if(NOT MSVC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug|RelWithDebInfo)
        # Strip unnecessary sections of the binary on Linux/macOS
        pybind11_strip(${_pythonbind_target_name})
    endif()
    # target_link_libraries(westbind west)
    target_link_libraries(${_pythonbind_target_name} PUBLIC pybind11::module ${target})
    list(APPEND __python_bindings ${_pythonbind_target_name})


endforeach(target $LIBRIR_TARGETS)



# install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/librir
# 	DESTINATION ${CMAKE_INSTALL_PREFIX}
# 	FILES_MATCHING PATTERN "*.pyd"
# )


install(FILES ${CMAKE_SOURCE_DIR}/CMakeLists.txt ${CMAKE_SOURCE_DIR}/README.md
		DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(DIRECTORY $${CMAKE_CURRENT_SOURCE_DIR}/librir/LICENSES 
		DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
		FILES_MATCHING PATTERN "*.txt"
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/librir/LICENSE 
		DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
)

install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/
		DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/libs
)

install(TARGETS ${_pythonbind_target_name} DESTINATION ${CMAKE_INSTALL_PREFIX}/python/${PROJECT_NAME}/libs)



install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/python
	FILES_MATCHING PATTERN "*.py"
)



# install(TARGETS ${LIBRIR_TARGETS} RUNTIME 
# 		DESTINATION ${CMAKE_INSTALL_PREFIX}/python/${PROJECT_NAME}/libs
# )
install(CODE "set(FFMPEG_LIB_DIR \"${FFMPEG_LIB_DIR}\")")
install(CODE "set(FFMPEG_INCLUDE_DIR \"${FFMPEG_INCLUDE_DIR}\")")
install(CODE "set(FFMPEG_LIBS \"${FFMPEG_LIBS}\")")
install(CODE "set(ADDITIONAL_FFMPEG_LIBS \"${ADDITIONAL_FFMPEG_LIBS}\")")
install(CODE "set(FFMPEG_PREFIX \"${FFMPEG_PREFIX}\")")
install(CODE "set(FFMPEG_SUFFIX \"${FFMPEG_SUFFIX}\")")
install(CODE "set(COMPLETE_FFMPEG_LIBS \"${COMPLETE_FFMPEG_LIBS}\")")
install(CODE "set(FFMPEG_INSTALL_DIR \"${CMAKE_INSTALL_PREFIX}/python/${PROJECT_NAME}/libs\")")
install(SCRIPT ../cpp/video_io/InstallFfmpeg.cmake)

install(IMPORTED_RUNTIME_ARTIFACTS ${LIBRIR_TARGETS} DESTINATION ${CMAKE_INSTALL_PREFIX}/python/${PROJECT_NAME}/libs)

if(MSVC OR WIN32)
set(SHARED_LIBRARY_SUFFIX_PATTERN "*.dll")
else()
set(SHARED_LIBRARY_SUFFIX_PATTERN "*.so*")
endif()
configure_file(pyproject.toml.in pyproject.toml @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pyproject.toml ${PROJECT_SOURCE_DIR}/README.md
		DESTINATION ${CMAKE_INSTALL_PREFIX}/python
)

# setting version in __init__.py

function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

set(INIT_PY_FILES)
file(REMOVE __init__.py.in)
file(WRITE __init__.py.in "")

cat( ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/__init__.py __init__.py.in)
file(APPEND __init__.py.in "\n__version__ = \"@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@\"")

configure_file(__init__.py.in __init__.py @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/__init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/python/${PROJECT_NAME}/)
FILE(REMOVE __init__.py.in)
install(TARGETS ${__python_bindings} DESTINATION ${CMAKE_INSTALL_PREFIX}/librir/${target})


# for development purposes
install(IMPORTED_RUNTIME_ARTIFACTS ${LIBRIR_TARGETS} 
	DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/libs
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pyproject.toml ${PROJECT_SOURCE_DIR}/README.md
		DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS ${__python_bindings} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}/libs)