# Find or fetch ZSTD
include(ExternalProject)
include(FetchContent)

set(TOOLS_DEP_LIBS)

list(APPEND TOOLS_DEP_LIBS libzstd_static)

# add_compile_definitions(BUILD_TOOLS_LIB)
if(BUILD_SHARED_LIBS)
    add_compile_definitions(BUILD_TOOLS_LIB)
endif()

set(LIBRIR_TOOLS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/FileAttributes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FileLock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Misc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ReadFileChunk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SIMD.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tools.cpp

    ${MINIZIP_SRC}
)

set(LIBRIR_TOOLS_SRC ${LIBRIR_TOOLS_SRC} PARENT_SCOPE)

set(TOOLS_HEADERS
    SIMD.h
    FileAttributes.h
    FileLock.h
    Log.h
    Misc.h
    ReadFileChunk.h
    tools.h
)

add_library(tools ${LIBRIR_TOOLS_SRC})
add_library(librir::tools ALIAS tools)

if(MSVC)
    target_compile_definitions(tools PUBLIC UNICODE _UNICODE)
endif()

if(NOT WIN32)
    list(APPEND TOOLS_DEP_LIBS pthread)
endif()

target_link_libraries(tools PRIVATE ${TOOLS_DEP_LIBS})
target_link_directories(tools PRIVATE ${ZSTD_LIB_DIR})

target_include_directories(tools PRIVATE $<BUILD_INTERFACE:${ZSTD_INCLUDE_DIR}>)

target_include_directories(tools
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${RIR_CONFIG_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_sources(
    tools
    PUBLIC
    FILE_SET tools_public_headers
    TYPE HEADERS
    FILES
    ${TOOLS_HEADERS}
)

target_sources(
    tools
    INTERFACE
    FILE_SET tools_api_header
    TYPE HEADERS
    FILES
    tools.h
)

target_include_directories(tools PRIVATE ${MINIZIP_INCLUDE_DIR})

# Configure and install PackageConfig files
generate_export_header(tools)
set_property(TARGET tools PROPERTY VERSION ${PROJECT_VERSION})
set_property(TARGET tools PROPERTY SOVERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET tools PROPERTY
    INTERFACE_tools_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET tools APPEND PROPERTY
    COMPATIBLE_INTERFACE_STRING tools_MAJOR_VERSION
)
get_target_property(TOOLS_PUBLIC_HEADERS tools INTERFACE_SOURCES)
install(TARGETS tools EXPORT ToolsTargets

    # LIBRARY DESTINATION lib
    # ARCHIVE DESTINATION lib
    # RUNTIME DESTINATION bin
    COMPONENT Tools
    INCLUDES DESTINATION include
    FILE_SET tools_public_headers
    FILE_SET tools_api_header DESTINATION include/api
)

# install(
# TARGETS ${LIBRIR_TARGETS}
# EXPORT ToolsTargets
# )
include(CMakePackageConfigHelpers)

configure_package_config_file(ToolsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ToolsConfig.cmake
    INSTALL_DESTINATION lib/cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ToolsConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

export(EXPORT ToolsTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/ToolsTargets.cmake"
    NAMESPACE librir::
)

set(ConfigPackageLocation lib/cmake/librir)

install(EXPORT ToolsTargets
    FILE ToolsTargets.cmake
    DESTINATION ${ConfigPackageLocation}
    NAMESPACE librir::
    EXPORT_LINK_INTERFACE_LIBRARIES
    COMPONENT Tools
)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ToolsConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/ToolsConfigVersion.cmake"
    DESTINATION
    ${ConfigPackageLocation}
    COMPONENT
    Devel
)
