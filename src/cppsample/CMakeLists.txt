cmake_minimum_required(VERSION 3.5)

project(cppsample)

include_directories(${PROJECT_SOURCE_DIR}/../..)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(cppsample cppsample.cpp)

target_link_libraries(cppsample um)
