#!/bin/bash
set -e

TEST_ROOT_PATH="$(dirname ${0})/.."
TEST_BUILD_FOLDER="build"
TEST_BUILD_FOLDER_PATH="${TEST_ROOT_PATH}/${TEST_BUILD_FOLDER}"

# Cleanup
rm -rf "${TEST_BUILD_FOLDER_PATH}"
mkdir "${TEST_BUILD_FOLDER_PATH}"
cd "${TEST_BUILD_FOLDER_PATH}"

# Build the test application
cmake .. -DENABLE_COVERAGE=true && cmake --build .

# Run the test application and collect code coverage data
# The html report can be found "build/coverage/index.html"
./libum_test && cmake --build . --target coverage

cd coverage
echo "Code coverage report: file://$(pwd)/index.html"
