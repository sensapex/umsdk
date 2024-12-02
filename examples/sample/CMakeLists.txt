cmake_minimum_required(VERSION 3.10)

project(
    sample
    LANGUAGES C
)

include_directories(${PROJECT_SOURCE_DIR}/../../inc)

link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

add_executable(sample sample.c)

target_link_libraries(sample um)

add_dependencies(sample um)
