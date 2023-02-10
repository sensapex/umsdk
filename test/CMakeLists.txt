cmake_minimum_required(VERSION 3.14)
project(libum_test_project
        LANGUAGES C CXX)

# If code coverage data is collected.
if(ENABLE_COVERAGE)
    # set compiler flags
    set(CMAKE_C_FLAGS "-O0 -coverage")

    # find required tools
    find_program(LCOV lcov REQUIRED)
    find_program(GENHTML genhtml REQUIRED)

    # add coverage target
    add_custom_target(coverage
        # gather data
        COMMAND ${LCOV} --directory . --capture --output-file coverage.info --include "**/src/*"
        # generate report
        COMMAND ${GENHTML} --demangle-cpp -o coverage coverage.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

# Build type definitions. Use Release by default.
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Available since cmake 3.24
if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)

# Recompile libum also
add_subdirectory(../src libum) #libum

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.13.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Find global header path
find_path(UM_INC
    NAMES libum.h
    PATHS
       ${PROJECT_SOURCE_DIR}/../include  # Local build
)

# Actual tests
enable_testing()

# Create test executable
add_executable(
    libum_test
    libum_c_test.cpp
    libum_cpp_test.cpp
)

# Set include dirs
target_include_directories(libum_test PRIVATE ${UM_INC})

# Link required libraries
target_link_libraries(libum_test
    PRIVATE
        GTest::gtest_main
        um
)

# Finally. Include test runner and test cases.
include(GoogleTest)
gtest_discover_tests(libum_test)
