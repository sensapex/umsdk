cmake_minimum_required(VERSION 3.25)

project(
    cppsample
    LANGUAGES CXX
    )

include_directories(${PROJECT_SOURCE_DIR}/../../include)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(cppsample cppsample.cpp)

target_link_libraries(cppsample um)
