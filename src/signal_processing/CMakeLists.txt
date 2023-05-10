add_definitions("-DBUILD_SIGNAL_PROCESSING_LIB")


get_target_property(TOOLS_HEADERS tools PUBLIC_HEADER)
get_target_property(TOOLS_HEADERS_DIR tools INCLUDE_DIRECTORIES)

get_target_property(GEOMETRY_HEADERS geometry PUBLIC_HEADER)
get_target_property(GEOMETRY_HEADERS_DIR geometry INCLUDE_DIRECTORIES)

# MESSAGE(STATUS ${GEOMETRY_HEADERS})
SET(LIBRIR_SIGNAL_PROCESSING_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/BadPixels.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Filters.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/signal_processing.cpp
	${CHARLS_SRC}
	${JPEG9B_SRC}
)

#~ include_directories(${CMAKE_CURRENT_SOURCE_DIR})
#~ include_directories(${CMAKE_CURRENT_SOURCE_DIR}/charls)
#~ include_directories(${CMAKE_CURRENT_SOURCE_DIR}/jpeg-9b)

#~ include_directories(${GEOMETRY_HEADERS_DIR})
#~ include_directories(${TOOLS_HEADERS_DIR})
#~ include_directories(${TOOLS_HEADERS_DIR})
# include_directories(JPEG9B_DIR CHARLS_DIR)

set(SIGNAL_PROCESSING_HEADERS 
	${CMAKE_CURRENT_SOURCE_DIR}/BadPixels.h
	${CMAKE_CURRENT_SOURCE_DIR}/Filters.h
	${CMAKE_CURRENT_SOURCE_DIR}/signal_processing.h
)

SET(LIBRIR_SIGNAL_PROCESSING_SRC ${LIBRIR_SIGNAL_PROCESSING_SRC} PARENT_SCOPE)

add_library(signal_processing ${LIBRIR_SIGNAL_PROCESSING_SRC})
target_link_libraries(signal_processing geometry tools)
target_include_directories(geometry PUBLIC 	
	${CMAKE_CURRENT_SOURCE_DIR}
	${JPEG9B_DIR}
	${CHARLS_DIR}
	)

set_target_properties(signal_processing PROPERTIES
	PUBLIC_HEADER "${SIGNAL_PROCESSING_HEADERS}"
	#~ LIBRARY_OUTPUT_DIRECTORY ${DESTINATION_DIR}
	#~ RUNTIME_OUTPUT_DIRECTORY ${DESTINATION_DIR}
)
