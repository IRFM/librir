
MACRO(SUBDIRLIST result curdir)
    FILE(GLOB children ${curdir}/*) # This was changed
    SET(dirlist "")
    FOREACH(child ${children})
        IF(IS_DIRECTORY ${child}) # This was changed
			get_filename_component(dirname ${child} NAME )
            LIST(APPEND dirlist ${dirname})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

SUBDIRLIST(_PRESENT_PLUGINS ${CMAKE_CURRENT_SOURCE_DIR})
MESSAGE(STATUS "_PRESENT_PLUGINS : " ${_PRESENT_PLUGINS})
set(EXCLUDED_PATTERNS)
foreach(plugin ${_PRESENT_PLUGINS})
        string(TOUPPER ${plugin} UPPERCASE_PLUGIN)
        if(DISABLE_${UPPERCASE_PLUGIN})
            # list(APPEND EXCLUDED_PATTERNS PATTERN ${plugin}* EXCLUDE)
            # set(PLUGIN_INSTALLED_FILES)
            # file(GLOB_RECURSE _TEMP ${CMAKE_INSTALL_PREFIX}/*${plugin}*)
            # set(PLUGIN_INSTALLED_FILES ${PLUGIN_INSTALLED_FILES} ${_TEMP})
            # file(GLOB _TEMP LIST_DIRECTORIES true ${CMAKE_INSTALL_PREFIX}/librir/${plugin} )
            # set(PLUGIN_INSTALLED_FILES ${PLUGIN_INSTALLED_FILES} ${_TEMP})
            # MESSAGE(STATUS "PLUGIN_INSTALLED_FILES : " ${PLUGIN_INSTALLED_FILES})
            # if(PLUGIN_INSTALLED_FILES)
            #     file( REMOVE_RECURSE ${PLUGIN_INSTALLED_FILES})
            # endif()
            continue()
        endif()


        add_subdirectory(${plugin})
        MESSAGE(STATUS "Include '${plugin}' plugin")
        list(APPEND PRESENT_PLUGINS ${plugin})
endforeach()


SET(EXCLUDED_PATTERNS ${EXCLUDED_PATTERNS} PARENT_SCOPE)
SET(PRESENT_PLUGINS ${PRESENT_PLUGINS} PARENT_SCOPE)
# add_subdirectory(test) 