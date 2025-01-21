# Generated from librir.pro.

cmake_minimum_required(VERSION 3.16)

project(import
	VERSION 0.0
	DESCRIPTION "import"
	LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

add_executable(import main.cpp)

find_package(librir REQUIRED)

target_include_directories(import PRIVATE ${LIBRIR_INCLUDE_DIR})
target_link_directories(import PRIVATE ${LIBRIR_LIB_DIRS})
target_link_libraries(import PRIVATE video_io)

