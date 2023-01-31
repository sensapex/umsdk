cmake_minimum_required(VERSION 3.25)

project(
    umsdk
    DESCRIPTION "Sensapex umsdk with examples."
    LANGUAGES CXX
    )

add_subdirectory(src) #umlib
add_subdirectory(examples) #Example applications
