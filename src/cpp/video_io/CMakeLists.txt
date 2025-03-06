option(USE_ZFILE "Whether to compile with ZSTD file format" OFF)

if(USE_ZFILE)
    add_compile_definitions(USE_ZFILE)
endif()

# add_compile_definitions(BUILD_IO_LIB)
if(BUILD_SHARED_LIBS)
    add_compile_definitions(BUILD_IO_LIB)
endif()

# Set sources
set(LIBRIR_IO_SRC
    BaseCalibration.cpp
    LutCalibration.cpp
    IRFileLoader.cpp
    h264.cpp
    IRVideoLoader.cpp
    HCCLoader.cpp
    video_io.cpp
)

if(USE_ZFILE)
    list(APPEND LIBRIR_IO_SRC ZFile.cpp)
endif()

set(LIBRIR_IO_SRC ${LIBRIR_IO_SRC} PARENT_SCOPE)

set(IO_HEADERS
    IRVideoLoader.h
    IRFileLoader.h
    BaseCalibration.h
    LutCalibration.h
    HCCLoader.h
    video_io.h
    h264.h
)

if(USE_ZFILE)
    list(APPEND LIBRIR_IO_SRC ZFile.h)
endif()

add_library(video_io ${LIBRIR_IO_SRC})
add_library(librir::video_io ALIAS video_io)

set(FFMPEG_LIBS ${FFMPEG_LIBRARIES} CACHE INTERNAL "")
set(ADDITIONAL_FFMPEG_LIBS ${ADDITIONAL_FFMPEG_LIBS} CACHE INTERNAL "")

# include and linking
target_link_libraries(video_io PUBLIC tools geometry signal_processing)
target_include_directories(video_io
    PRIVATE $<BUILD_INTERFACE:${FFMPEG_INCLUDE_DIRS}>
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(video_io
    PRIVATE
    $<BUILD_INTERFACE:${FFMPEG_SHARED_LIBRARIES}>

    # INTERFACE
    # $<BUILD_INTERFACE:${FFMPEG_ADDITIONAL_DEPENDENCIES}>
)

target_sources(
    video_io
    PUBLIC
    FILE_SET video_io_public_headers
    TYPE HEADERS
    FILES
    ${IO_HEADERS}
)

target_sources(
    video_io
    INTERFACE
    FILE_SET video_io_api_header
    TYPE HEADERS
    FILES
    video_io.h
)

# Configure and install PackageConfig files
generate_export_header(video_io)
set_property(TARGET video_io PROPERTY VERSION ${PROJECT_VERSION})
set_property(TARGET video_io PROPERTY SOVERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET video_io PROPERTY
    INTERFACE_video_io_MAJOR_VERSION ${PROJECT_VERSION_MAJOR})
set_property(TARGET video_io APPEND PROPERTY
    COMPATIBLE_INTERFACE_STRING video_io_MAJOR_VERSION
)

install(TARGETS video_io EXPORT VideoIOTargets
    # LIBRARY DESTINATION lib
    # ARCHIVE DESTINATION lib
    # RUNTIME DESTINATION bin
    COMPONENT VideoIO
    INCLUDES DESTINATION include
    FILE_SET video_io_public_headers
    FILE_SET video_io_api_header DESTINATION include/api
)

# install( 
# TARGETS ${LIBRIR_TARGETS}
# EXPORT VideoIOTargets
# )
include(CMakePackageConfigHelpers)

configure_package_config_file(VideoIOConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/VideoIOConfig.cmake
    INSTALL_DESTINATION lib/cmake
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/VideoIOConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

export(EXPORT VideoIOTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/VideoIOTargets.cmake"
    NAMESPACE librir::
)

set(ConfigPackageLocation lib/cmake/librir)

install(EXPORT VideoIOTargets
    FILE VideoIOTargets.cmake
    DESTINATION ${ConfigPackageLocation}
    NAMESPACE librir::
    EXPORT_LINK_INTERFACE_LIBRARIES
)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/VideoIOConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/VideoIOConfigVersion.cmake"
    DESTINATION
    ${ConfigPackageLocation}
    COMPONENT
    Devel
)
