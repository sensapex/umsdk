cmake_minimum_required(VERSION 3.5)

project(sample)

include_directories(${PROJECT_SOURCE_DIR}/../..)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(sample sample.c)

target_link_libraries(sample um)
