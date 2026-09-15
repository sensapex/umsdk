cmake_minimum_required(VERSION 3.10)

project(
    umsdk
    DESCRIPTION "Sensapex umsdk with examples."
    )

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/bin>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shared>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "$<1:${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/static>")

# Option to include tests
option(BUILD_TESTS "Build the tests" OFF)

add_subdirectory(src)
add_subdirectory(examples)

if(BUILD_TESTS)
    add_subdirectory(test) # Unit tests
endif()
