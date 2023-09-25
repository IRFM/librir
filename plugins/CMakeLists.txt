
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

function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()

set(INIT_PY_FILES)
file(REMOVE __init__.py.in)
file(WRITE __init__.py.in "")
cat(${CMAKE_SOURCE_DIR}/src/python/librir/__init__.py __init__.py.in )

foreach(plugin ${_PRESENT_PLUGINS})
        string(TOUPPER ${plugin} UPPERCASE_PLUGIN)
        if(DISABLE_${UPPERCASE_PLUGIN})
            continue()
        endif()


        add_subdirectory(${plugin})
        MESSAGE(STATUS "Include '${plugin}' plugin")
        list(APPEND PRESENT_PLUGINS ${plugin})
endforeach()

# # Call the "cat" function for each input file
foreach(INIT_PY_FILE ${INIT_PY_FILES})
  cat(${INIT_PY_FILE} __init__.py.in)
endforeach()


# Copy the temporary file to the final location
configure_file(__init__.py.in __init__.py @ONLY)


install(FILES  ${CMAKE_CURRENT_BINARY_DIR}/__init__.py 
		DESTINATION ${CMAKE_INSTALL_PREFIX}/librir)


SET(EXCLUDED_PATTERNS ${EXCLUDED_PATTERNS} PARENT_SCOPE)
SET(PRESENT_PLUGINS ${PRESENT_PLUGINS} PARENT_SCOPE)
# add_subdirectory(test)  