cmake_minimum_required(VERSION 3.14)
project(libum_test_project
        LANGUAGES C CXX)

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

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Set include folder
find_path(UM_INC
    NAMES libum.h
    PATHS
       ${PROJECT_SOURCE_DIR}/../include  # Local build
)
message(STATUS "UM_INC => ${UM_INC}")

# Actual tests
enable_testing()

add_executable(
    libum_test
    libum_c_test.cpp
    libum_cpp_test.cpp
)

# Set include dirs
target_include_directories(libum_test PRIVATE ${UM_INC})

# Find and set the library under testing (libum)
find_library(UM_LIB NAMES um
    PATHS
        ${PROJECT_SOURCE_DIR}/../build-${CMAKE_BUILD_TYPE}/src  # Local build
        ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR} # System installed
)
target_link_libraries(libum_test
    PRIVATE
        GTest::gtest_main
        ${UM_LIB}
)

include(GoogleTest)
gtest_discover_tests(libum_test)
