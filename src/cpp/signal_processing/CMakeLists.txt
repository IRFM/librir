add_definitions("-DBUILD_SIGNAL_PROCESSING_LIB")

# get previous targets include folders
get_target_property(TOOLS_HEADERS tools PUBLIC_HEADER)
get_target_property(TOOLS_HEADERS_DIR tools INCLUDE_DIRECTORIES)

get_target_property(GEOMETRY_HEADERS geometry PUBLIC_HEADER)
get_target_property(GEOMETRY_HEADERS_DIR geometry INCLUDE_DIRECTORIES)


SET(LIBRIR_SIGNAL_PROCESSING_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/BadPixels.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Filters.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/signal_processing.cpp
	# ${CHARLS_SRC}
	${JPEG9B_SRC}
)

set(SIGNAL_PROCESSING_HEADERS 
	${CMAKE_CURRENT_SOURCE_DIR}/BadPixels.h
	${CMAKE_CURRENT_SOURCE_DIR}/Filters.h
	${CMAKE_CURRENT_SOURCE_DIR}/signal_processing.h
)

SET(LIBRIR_SIGNAL_PROCESSING_SRC ${LIBRIR_SIGNAL_PROCESSING_SRC} PARENT_SCOPE)

add_library(signal_processing ${LIBRIR_SIGNAL_PROCESSING_SRC})
target_link_libraries(signal_processing geometry tools)
target_include_directories(	signal_processing 
							INTERFACE 	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
										$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(signal_processing PRIVATE ${JPEG9B_DIR} ${CHARLS_DIR})


set_target_properties(signal_processing PROPERTIES
	PUBLIC_HEADER "${SIGNAL_PROCESSING_HEADERS}"
)
