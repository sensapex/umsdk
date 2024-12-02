cmake_minimum_required(VERSION 3.10)

project(examples)

add_subdirectory(cppsample)
add_subdirectory(sample)

# Create a custom target for examples
add_custom_target(examples ALL DEPENDS cppsample sample)
