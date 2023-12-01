add_definitions("-DBUILD_GEOMETRY_LIB")

# get previous target include folders
get_target_property(TOOLS_HEADERS tools PUBLIC_HEADER)
get_target_property(TOOLS_HEADERS_DIR tools INCLUDE_DIRECTORIES)

SET(LIBRIR_GEOMETRY_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/geometry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Polygon.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DrawPolygon.cpp
)

SET(LIBRIR_GEOMETRY_SRC ${LIBRIR_GEOMETRY_SRC} PARENT_SCOPE)


add_library(geometry ${LIBRIR_GEOMETRY_SRC})

set(GEOMETRY_HEADERS 
	${CMAKE_CURRENT_SOURCE_DIR}/Primitives.h
	${CMAKE_CURRENT_SOURCE_DIR}/Polygon.h
	${CMAKE_CURRENT_SOURCE_DIR}/DrawPolygon.h
	${CMAKE_CURRENT_SOURCE_DIR}/geometry.h
)

set(GEOMETRY_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(	geometry 
							INTERFACE 	$<BUILD_INTERFACE:${GEOMETRY_INCLUDE_DIRS}>
										$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(geometry tools)

set_target_properties(geometry PROPERTIES
	PUBLIC_HEADER "${GEOMETRY_HEADERS}"
)

