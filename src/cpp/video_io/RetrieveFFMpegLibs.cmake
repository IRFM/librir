


# retrieve ffmpeg dependency manually since there is no easy way 
# to fetch and compile sources for windows and linux
# FIXME: FetchContent/find_package ffmpeg
IF(MSVC)
    SET(FFMPEG_LIB_DIR ${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg-4.3-msvc/lib)
    SET(FFMPEG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg-4.3-msvc)
    SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
    SET(ADDITIONAL_FFMPEG_LIBS libx264 libx265 kvazaar postproc vcruntime140_1)
    SET(FFMPEG_SYSTEM_DEPENDENCIES Vfw32 shlwapi strmbase)
    SET(FFMPEG_PREFIX)
    SET(FFMPEG_SUFFIX .dll)
    SET(FFMPEG_IMPLIB .lib)

ELSE()
    IF(WIN32)
        # mingw
        SET(FFMPEG_LIB_DIR ${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg/install/lib)
        SET(FFMPEG_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/3rd_64/ffmpeg/install/include)
        SET(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
        SET(ADDITIONAL_FFMPEG_LIBS postproc)
        SET(FFMPEG_SYSTEM_DEPENDENCIES)

        SET(FFMPEG_PREFIX)
        SET(FFMPEG_SUFFIX .dll)
        SET(FFMPEG_IMPLIB .lib)

    ENDIF()
    ExternalProject_Get_property(ffmpeg INSTALL_DIR)
    set(FFMPEG_LIB_DIR ${INSTALL_DIR}/lib)
    set(FFMPEG_INCLUDE_DIR ${INSTALL_DIR}/include)
    set(FFMPEG_LIBS avdevice avformat avcodec swscale avutil swresample avfilter)
    set(ADDITIONAL_FFMPEG_LIBS postproc)
    set(FFMPEG_SYSTEM_DEPENDENCIES)
    set(FFMPEG_PREFIX lib)
    set(FFMPEG_SUFFIX .so)
    set(FFMPEG_IMPLIB .a)
ENDIF()

MESSAGE(STATUS "Using ${FFMPEG_LIB_DIR} for ffmpeg library files")
set(COMPLETE_FFMPEG_LIBS ${FFMPEG_LIBS} ${ADDITIONAL_FFMPEG_LIBS})
set(FFMPEG_DLLS_DEPEDENCIES "" CACHE INTERNAL "")

foreach(ffmpeg_lib ${FFMPEG_LIBS})
    file(GLOB CORRECT_FFMPEG_SHARED_LIB_PATH_LIST "${FFMPEG_LIB_DIR}/*${ffmpeg_lib}*${FFMPEG_SUFFIX}*")
    list(GET CORRECT_FFMPEG_SHARED_LIB_PATH_LIST 0 CORRECT_FFMPEG_SHARED_LIB_PATH)
    file(GLOB CORRECT_FFMPEG_IMPLIB_PATH_LIST "${FFMPEG_LIB_DIR}/*${ffmpeg_lib}*${FFMPEG_IMPLIB}*")
    if(CORRECT_FFMPEG_IMPLIB_PATH_LIST)
        list(GET CORRECT_FFMPEG_IMPLIB_PATH_LIST 0 CORRECT_FFMPEG_IMPLIB_PATH)
    endif()

    add_library(${ffmpeg_lib} SHARED IMPORTED)

    set_target_properties(${ffmpeg_lib} PROPERTIES
        IMPORTED_LOCATION ${CORRECT_FFMPEG_SHARED_LIB_PATH}

    )
    if(WIN32 OR MSVC)
        get_filename_component(__dll_name ${CORRECT_FFMPEG_SHARED_LIB_PATH} NAME)
        set_target_properties(${ffmpeg_lib} PROPERTIES
            RUNTIME_OUTPUT_NAME ${__dll_name}
        )
    endif()
    if(CORRECT_FFMPEG_IMPLIB_PATH)
        set_target_properties(${ffmpeg_lib} PROPERTIES
            IMPORTED_IMPLIB ${CORRECT_FFMPEG_IMPLIB_PATH}
        )
    endif()
    set_target_properties(${ffmpeg_lib} PROPERTIES
        INCLUDE_DIRECTORIES ${FFMPEG_INCLUDE_DIR}/lib${ffmpeg_lib}
    )
    list(APPEND FFMPEG_DLLS_DEPEDENCIES ${CORRECT_FFMPEG_SHARED_LIB_PATH})

endforeach()
foreach(_additional_ffmpeg_lib ${ADDITIONAL_FFMPEG_LIBS})
    file(GLOB CORRECT_FFMPEG_SHARED_LIB_PATH "${FFMPEG_LIB_DIR}/*${_additional_ffmpeg_lib}*${FFMPEG_SUFFIX}*")
    list(APPEND FFMPEG_DLLS_DEPEDENCIES ${CORRECT_FFMPEG_SHARED_LIB_PATH})
endforeach()
# endif()

set(FFMPEG_DLLS_DEPEDENCIES ${FFMPEG_DLLS_DEPEDENCIES} CACHE INTERNAL "")
set(FFMPEG_LIB_DIR ${FFMPEG_LIB_DIR} CACHE INTERNAL "")
set(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR} CACHE INTERNAL "")
set(FFMPEG_LIBS ${FFMPEG_LIBS} CACHE INTERNAL "")
set(ADDITIONAL_FFMPEG_LIBS ${ADDITIONAL_FFMPEG_LIBS} CACHE INTERNAL "")
set(FFMPEG_PREFIX ${FFMPEG_PREFIX} CACHE INTERNAL "")
set(FFMPEG_SUFFIX ${FFMPEG_SUFFIX} CACHE INTERNAL "")
set(COMPLETE_FFMPEG_LIBS ${COMPLETE_FFMPEG_LIBS} CACHE INTERNAL "")
