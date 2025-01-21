# add_compile_definitions(BUILD_GEOMETRY_LIB)

if(BUILD_SHARED_LIBS)
	add_compile_definitions(BUILD_GEOMETRY_LIB)
endif()

SET(LIBRIR_GEOMETRY_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/geometry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Polygon.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DrawPolygon.cpp
)

add_library(geometry ${LIBRIR_GEOMETRY_SRC})
add_library(librir::geometry ALIAS geometry)

set(GEOMETRY_HEADERS
	Primitives.h
	Polygon.h
	DrawPolygon.h
	geometry.h
)

target_include_directories(geometry
	PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(geometry PUBLIC tools)

target_sources(
	geometry
	PUBLIC
	FILE_SET geometry_public_headers
	TYPE HEADERS
	FILES
	${GEOMETRY_HEADERS}
)

target_sources(
	geometry
	INTERFACE
	FILE_SET geometry_api_header
	TYPE HEADERS
	FILES
	geometry.h
)

# Configure and install PackageConfig files
generate_export_header(geometry)
set_property(TARGET geometry PROPERTY VERSION ${PROJECT_VERSION})
set_property(TARGET geometry PROPERTY SOVERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET geometry PROPERTY
	INTERFACE_geometry_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET geometry APPEND PROPERTY
	COMPATIBLE_INTERFACE_STRING geometry_MAJOR_VERSION
)

install(TARGETS geometry EXPORT GeometryTargets
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	COMPONENT Geometry
	RUNTIME DESTINATION bin
	INCLUDES DESTINATION include
	FILE_SET geometry_public_headers
	FILE_SET geometry_api_header DESTINATION include/api
)

# install(
# TARGETS ${LIBRIR_TARGETS}
# EXPORT GeometryTargets
# )
include(CMakePackageConfigHelpers)

configure_package_config_file(GeometryConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/GeometryConfig.cmake
	INSTALL_DESTINATION lib/cmake
)

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/GeometryConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion)

export(EXPORT GeometryTargets
	FILE "${CMAKE_CURRENT_BINARY_DIR}/GeometryTargets.cmake"
	NAMESPACE librir::
)

set(ConfigPackageLocation lib/cmake/librir)

install(EXPORT GeometryTargets
	FILE GeometryTargets.cmake
	DESTINATION ${ConfigPackageLocation}
	NAMESPACE librir::
	EXPORT_LINK_INTERFACE_LIBRARIES
)

install(
	FILES
	${CMAKE_CURRENT_BINARY_DIR}/GeometryConfig.cmake
	"${CMAKE_CURRENT_BINARY_DIR}/GeometryConfigVersion.cmake"
	DESTINATION
	${ConfigPackageLocation}
	COMPONENT
	Devel
)