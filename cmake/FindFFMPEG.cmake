#[==[
Provides the following variables:

  * `FFMPEG_INCLUDE_DIRS`: Include directories necessary to use FFMPEG.
  * `FFMPEG_LIBRARIES`: Libraries necessary to use FFMPEG. Note that this only
    includes libraries for the components requested.
  * `FFMPEG_VERSION`: The version of FFMPEG found.

The following components are supported:

  * `avcodec`
  * `avdevice`
  * `avfilter`
  * `avformat`
  * `avresample`
  * `avutil`
  * `swresample`
  * `swscale`

For each component, the following are provided:

  * `FFMPEG_<component>_FOUND`: Libraries for the component.
  * `FFMPEG_<component>_INCLUDE_DIRS`: Include directories for
    the component.
  * `FFMPEG_<component>_LIBRARIES`: Libraries for the component.
  * `FFMPEG::<component>`: A target to use with `target_link_libraries`.

Note that only components requested with `COMPONENTS` or `OPTIONAL_COMPONENTS`
are guaranteed to set these variables or provide targets.
#]==]

if(NOT FFMPEG_FIND_COMPONENTS)
  # The default components were taken from a survey over other FindFFMPEG.cmake files
  set(FFMPEG_FIND_COMPONENTS avcodec avdevice avfilter avformat avresample avutil swresample swscale)
endif()

set(_INCLUDE_SEARCH_PATHS
  "${FFMPEG_ROOT}/include"
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include # Fink
  /opt/local/include # DarwinPorts
  /opt/csw/include # Blastwave
  /opt/include
  /usr/freeware/include
)
set(_LIB_SEARCH_PATHS
  "${FFMPEG_ROOT}/lib"
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/lib
  /usr/local/lib64
  /usr/lib
  /usr/lib64
  /sw/lib
  /opt/local/lib
  /opt/csw/lib
  /opt/lib
  /usr/freeware/lib64
  "${FFMPEG_ROOT}/bin"
)

if(MSVC OR WIN32)
  set(_SHARED_PREFIX ".dll")
else()
  set(_SHARED_PREFIX ".so")
endif()

function(_import_shared_with_glob component _lib_dir _lib_type)
  file(GLOB FFMPEG_${component}_SHARED_LIBRARY_LOCATION ${_lib_dir}/${component}*${_SHARED_PREFIX})

  if(FFMPEG_${component}_SHARED_LIBRARY_LOCATION)
    set(FFMPEG_${component}_SHARED_LIBRARY_LOCATION ${FFMPEG_${component}_SHARED_LIBRARY_LOCATION} CACHE INTERNAL "")
    message(STATUS "${component}")
    add_library("FFMPEG::${component}_shared" ${_lib_type} IMPORTED GLOBAL)
    set_target_properties("FFMPEG::${component}_shared" PROPERTIES
      IMPORTED_LOCATION "${FFMPEG_${component}_SHARED_LIBRARY_LOCATION}"
      INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_${component}_INCLUDE_DIR}"
      IMPORTED_IMPLIB "${FFMPEG_${component}_LIBRARY}"

      # IMPORTED_LINK_INTERFACE_LIBRARIES "${_deps_link}"
    )
  endif()
endfunction()

function(_ffmpeg_find component headername)
  find_path("FFMPEG_${component}_INCLUDE_DIR"
    NAMES
    "lib${component}/${headername}"
    PATHS
    ${_INCLUDE_SEARCH_PATHS}
    PATH_SUFFIXES
    ffmpeg
    DOC "FFMPEG's ${component} include directory")
  mark_as_advanced("FFMPEG_${component}_INCLUDE_DIR")

  # On Windows, static FFMPEG is sometimes built as `lib<name>.a`.
  if(WIN32)
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
    list(APPEND CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
  endif()

  find_library("FFMPEG_${component}_LIBRARY"
    NAMES
    "${component}"
    PATHS
    ${_LIB_SEARCH_PATHS}
    DOC "FFMPEG's ${component} library")

  # mark_as_advanced("FFMPEG_${component}_LIBRARY")
  set(FFMPEG_${component}_SHARED_LIBRARY)

  # Find shared libraries in different search paths
  foreach(_lib_dir IN LISTS _LIB_SEARCH_PATHS)
    _import_shared_with_glob(${component} ${_lib_dir} SHARED)

    if(TARGET "FFMPEG::${component}_shared")
      break()
    endif()

    # file(GLOB FFMPEG_${component}_SHARED_LIBRARY ${_lib_dir}/${component}*${_SHARED_PREFIX})
    # if( FFMPEG_${component}_SHARED_LIBRARY)
    # set(FFMPEG_${component}_SHARED_LIBRARY ${FFMPEG_${component}_SHARED_LIBRARY} CACHE INTERNAL "")

    # add_library("FFMPEG::${component}_shared" SHARED IMPORTED)
    # set_target_properties("FFMPEG::${component}_shared" PROPERTIES
    # IMPORTED_LOCATION "${FFMPEG_${component}_SHARED_LIBRARY}"
    # INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_${component}_INCLUDE_DIR}"
    # # IMPORTED_LINK_INTERFACE_LIBRARIES "${_deps_link}"
    # )
    # break()
    # endif()
  endforeach()

  # mark_as_advanced("FFMPEG_${component}_SHARED_LIBRARY")
  if(FFMPEG_${component}_LIBRARY AND FFMPEG_${component}_INCLUDE_DIR)
    set(_deps_found TRUE)
    set(_deps_link)

    foreach(_ffmpeg_dep IN LISTS ARGN)
      if(TARGET "FFMPEG::${_ffmpeg_dep}")
        list(APPEND _deps_link "FFMPEG::${_ffmpeg_dep}")
      else()
        set(_deps_found FALSE)
      endif()
    endforeach()

    if(_deps_found)
      if(NOT TARGET "FFMPEG::${component}")
        add_library("FFMPEG::${component}" UNKNOWN IMPORTED)
        set_target_properties("FFMPEG::${component}" PROPERTIES
          IMPORTED_LOCATION "${FFMPEG_${component}_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_${component}_INCLUDE_DIR}"
          IMPORTED_LINK_INTERFACE_LIBRARIES "${_deps_link}")
      endif()

      set("FFMPEG_${component}_FOUND" 1
        PARENT_SCOPE)

      set(version_header_path "${FFMPEG_${component}_INCLUDE_DIR}/lib${component}/version.h")

      if(EXISTS "${version_header_path}")
        string(TOUPPER "${component}" component_upper)
        file(STRINGS "${version_header_path}" version
          REGEX "#define  *LIB${component_upper}_VERSION_(MAJOR|MINOR|MICRO) ")
        string(REGEX REPLACE ".*_MAJOR *\([0-9]*\).*" "\\1" major "${version}")
        string(REGEX REPLACE ".*_MINOR *\([0-9]*\).*" "\\1" minor "${version}")
        string(REGEX REPLACE ".*_MICRO *\([0-9]*\).*" "\\1" micro "${version}")

        if(NOT major STREQUAL "" AND
          NOT minor STREQUAL "" AND
          NOT micro STREQUAL "")
          set("FFMPEG_${component}_VERSION" "${major}.${minor}.${micro}"
            PARENT_SCOPE)
        endif()
      endif()
    else()
      set("FFMPEG_${component}_FOUND" 0
        PARENT_SCOPE)
      set(what)

      if(NOT FFMPEG_${component}_LIBRARY)
        set(what "library")
      endif()

      if(NOT FFMPEG_${component}_INCLUDE_DIR)
        if(what)
          string(APPEND what " or headers")
        else()
          set(what "headers")
        endif()
      endif()

      set("FFMPEG_${component}_NOT_FOUND_MESSAGE"
        "Could not find the ${what} for ${component}."
        PARENT_SCOPE)
    endif()
  endif()
endfunction()

_ffmpeg_find(avutil avutil.h)
_ffmpeg_find(avresample avresample.h
  avutil)
_ffmpeg_find(swresample swresample.h
  avutil)
_ffmpeg_find(swscale swscale.h
  avutil)
_ffmpeg_find(avcodec avcodec.h
  avutil)
_ffmpeg_find(avformat avformat.h
  avcodec avutil)
_ffmpeg_find(avfilter avfilter.h
  avutil)
_ffmpeg_find(avdevice avdevice.h
  avformat avutil)

if(TARGET FFMPEG::avutil)
  set(_ffmpeg_version_header_path "${FFMPEG_avutil_INCLUDE_DIR}/libavutil/ffversion.h")

  if(EXISTS "${_ffmpeg_version_header_path}")
    file(STRINGS "${_ffmpeg_version_header_path}" _ffmpeg_version
      REGEX "FFMPEG_VERSION")
    string(REGEX REPLACE ".*\"n?\(.*\)\"" "\\1" FFMPEG_VERSION "${_ffmpeg_version}")
    unset(_ffmpeg_version)
  else()
    set(FFMPEG_VERSION FFMPEG_VERSION-NOTFOUND)
  endif()

  unset(_ffmpeg_version_header_path)

  unset(FFMPEG_ADDITIONAL_DEPENDENCIES)
  set(FFMPEG_ADDITIONAL_DEPENDENCIES "" CACHE INTERNAL "" FORCE)

  foreach(_additional_dep IN LISTS ADDITIONAL_FFMPEG_DEPS)
    foreach(_lib_dir IN LISTS _LIB_SEARCH_PATHS)
      _import_shared_with_glob(${_additional_dep} ${_lib_dir} MODULE)

      if(TARGET FFMPEG::${_additional_dep}_shared)
        list(APPEND FFMPEG_ADDITIONAL_DEPENDENCIES FFMPEG::${_additional_dep}_shared)

        # list(APPEND FFMPEG_ADDITIONAL_DEPENDENCIES ${${_additional_dep}_SHARED_LIBRARY})
        break()
      endif()
    endforeach()
  endforeach()

  set(FFMPEG_ADDITIONAL_DEPENDENCIES ${FFMPEG_ADDITIONAL_DEPENDENCIES} CACHE INTERNAL "" FORCE)

  # set_target_properties(FFMPEG::avutil PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES "${FFMPEG_ADDITIONAL_DEPENDENCIES}")

  # # target_link_libraries(FFMPEG::avutil INTERFACE ADDITIONAL_FFMPEG_DEPS)
endif()

set(FFMPEG_INCLUDE_DIRS "")
set(FFMPEG_LIBRARIES "")
set(FFMPEG_SHARED_LIBRARIES "")
set(_ffmpeg_required_vars "")

foreach(_ffmpeg_component IN LISTS FFMPEG_FIND_COMPONENTS)
  if(TARGET "FFMPEG::${_ffmpeg_component}_shared")
    list(APPEND FFMPEG_SHARED_LIBRARIES
      "FFMPEG::${_ffmpeg_component}_shared")
  endif()

  if(TARGET "FFMPEG::${_ffmpeg_component}")
    set(FFMPEG_${_ffmpeg_component}_INCLUDE_DIRS
      "${FFMPEG_${_ffmpeg_component}_INCLUDE_DIR}")
    set(FFMPEG_${_ffmpeg_component}_LIBRARIES
      "${FFMPEG_${_ffmpeg_component}_LIBRARY}")
    list(APPEND FFMPEG_INCLUDE_DIRS
      "${FFMPEG_${_ffmpeg_component}_INCLUDE_DIRS}")
    list(APPEND FFMPEG_LIBRARIES
      "${FFMPEG_${_ffmpeg_component}_LIBRARIES}")

    if(FFMEG_FIND_REQUIRED_${_ffmpeg_component})
      list(APPEND _ffmpeg_required_vars
        "FFMPEG_${_ffmpeg_required_vars}_INCLUDE_DIRS"
        "FFMPEG_${_ffmpeg_required_vars}_LIBRARIES")
    endif()
  endif()
endforeach()

unset(_ffmpeg_component)

if(FFMPEG_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFMPEG
  REQUIRED_VARS FFMPEG_INCLUDE_DIRS FFMPEG_LIBRARIES ${_ffmpeg_required_vars}
  VERSION_VAR FFMPEG_VERSION
  HANDLE_COMPONENTS)
unset(_ffmpeg_required_vars)
