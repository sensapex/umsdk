cmake_minimum_required(VERSION 3.10)

project(
        libum
        DESCRIPTION "Sensapex um-library"
        LANGUAGES C CXX
)

# Build type definitions. Use Release by default.
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif ()

# Compiler flags
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra" CACHE STRING "" FORCE)
#set(CMAKE_C_FLAGS_RELEASE "-O1" CACHE STRING "" FORCE)
add_compile_options(-DLIBUM_LIBRARY)

# Compile an Universal binary for MacOS (when not running code coverage build)
if (NOT ENABLE_COVERAGE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif ()

include_directories(${PROJECT_SOURCE_DIR}/../inc)

add_library(um SHARED libum.c)
add_library(um_static STATIC libum.c)

if (WIN32)
    # For gcc
    target_link_libraries(um ws2_32)
endif ()

install(TARGETS um um_static DESTINATION $ENV{HOME}/lib)
