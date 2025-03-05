# add_compile_definitions(BUILD_SIGNAL_PROCESSING_LIB)

set(LIBRIR_SIGNAL_PROCESSING_SRC
    BadPixels.cpp
    Filters.cpp
    signal_processing.cpp
)

set(SIGNAL_PROCESSING_HEADERS
    BadPixels.h
    Filters.h
    signal_processing.h
)

set(LIBRIR_SIGNAL_PROCESSING_SRC ${LIBRIR_SIGNAL_PROCESSING_SRC} PARENT_SCOPE)

if(BUILD_SHARED_LIBS)
    add_compile_definitions(BUILD_SIGNAL_PROCESSING_LIB)
endif()

add_library(signal_processing ${LIBRIR_SIGNAL_PROCESSING_SRC})
add_library(librir::signal_processing ALIAS signal_processing)

target_link_libraries(signal_processing PUBLIC geometry tools)
target_include_directories(signal_processing
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

# target_include_directories(signal_processing PRIVATE ${JPEG9B_DIR} ${CHARLS_DIR})
# set_target_properties(signal_processing PROPERTIES
# PUBLIC_HEADER "${SIGNAL_PROCESSING_HEADERS}"
# )
target_sources(
    signal_processing
    PUBLIC
    FILE_SET signal_processing_public_headers
    TYPE HEADERS
    FILES
    ${SIGNAL_PROCESSING_HEADERS}
)

target_sources(
    signal_processing
    INTERFACE
    FILE_SET signal_processing_api_header
    TYPE HEADERS
    FILES
    signal_processing.h
)

# Configure and install PackageConfig files
generate_export_header(signal_processing)
set_property(TARGET signal_processing PROPERTY VERSION ${PROJECT_VERSION})
set_property(TARGET signal_processing PROPERTY SOVERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET signal_processing PROPERTY
    INTERFACE_signal_processing_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET signal_processing APPEND PROPERTY
    COMPATIBLE_INTERFACE_STRING signal_processing_MAJOR_VERSION
)

install(TARGETS signal_processing EXPORT SignalProcessingTargets
    # LIBRARY DESTINATION lib
    # ARCHIVE DESTINATION lib
    # RUNTIME DESTINATION bin
    COMPONENT SignalProcessing
    INCLUDES DESTINATION include
    FILE_SET signal_processing_public_headers
    FILE_SET signal_processing_api_header DESTINATION include/api
)

# install(
# TARGETS ${LIBRIR_TARGETS}
# EXPORT SignalProcessingTargets
# )
include(CMakePackageConfigHelpers)

configure_package_config_file(SignalProcessingConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SignalProcessingConfig.cmake
    INSTALL_DESTINATION lib/cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SignalProcessingConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

export(EXPORT SignalProcessingTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/SignalProcessingTargets.cmake"
    NAMESPACE librir::
)

set(ConfigPackageLocation lib/cmake/librir)

install(EXPORT SignalProcessingTargets
    FILE SignalProcessingTargets.cmake
    DESTINATION ${ConfigPackageLocation}
    NAMESPACE librir::
    EXPORT_LINK_INTERFACE_LIBRARIES
)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SignalProcessingConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/SignalProcessingConfigVersion.cmake"
    DESTINATION
    ${ConfigPackageLocation}
    COMPONENT
    Devel
)
