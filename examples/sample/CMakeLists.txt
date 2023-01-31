cmake_minimum_required(VERSION 3.25)

project(
    sample
    LANGUAGES C
)

include_directories(${PROJECT_SOURCE_DIR}/../../include)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_executable(sample sample.c)

target_link_libraries(sample um)
