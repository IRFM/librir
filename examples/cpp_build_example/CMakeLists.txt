# Generated from librir.pro.

cmake_minimum_required(VERSION 3.16)

project(test)

list(APPEND CMAKE_PREFIX_PATH "/path/to/librir/install/lib/cmake/librir")

find_package(librir REQUIRED)

include_directories("${LIBRIR_INCLUDE_DIR}")
link_directories("${LIBRIR_LIB_DIRS}")
add_executable(test test.cpp)
target_link_libraries(test ${LIBRIR_LIBRARIES} )