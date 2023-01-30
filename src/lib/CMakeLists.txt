cmake_minimum_required(VERSION 3.5)

# Universal binaries for MacOS
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)

project(umsdk)

include_directories(${PROJECT_SOURCE_DIR}/../..)

add_library(um SHARED libum.c)

# TODO install definitions
install(TARGETS um DESTINATION lib)
