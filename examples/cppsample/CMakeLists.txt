cmake_minimum_required(VERSION 3.10)

project(
    cppsample
    LANGUAGES CXX
    )

include_directories(${PROJECT_SOURCE_DIR}/../../inc)

link_directories(${CMAKE_INSTALL_PREFIX}/lib ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

add_executable(cppsample cppsample.cpp)

target_link_libraries(cppsample um)

add_dependencies(cppsample um)
