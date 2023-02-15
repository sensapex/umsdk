#!/bin/bash
set -e

CLEAN=0

while getopts 'c' OPTION; do
  case "$OPTION" in
    c)
      echo "Cleanup build dir"
      CLEAN=1
      ;;
    ?)
      echo "script usage: $(basename ${0}) [-c]" >&2
      exit 1
      ;;
  esac
done

TEST_ROOT_PATH="$(dirname ${0})/.."
TEST_BUILD_FOLDER="build"
TEST_BUILD_FOLDER_PATH="${TEST_ROOT_PATH}/${TEST_BUILD_FOLDER}"

if [[ ${CLEAN} == 1 ]]
then
    rm -rf "${TEST_BUILD_FOLDER_PATH}"
    mkdir -p "${TEST_BUILD_FOLDER_PATH}"
fi

cd "${TEST_BUILD_FOLDER_PATH}"

# Build the test application
cmake .. -DENABLE_COVERAGE=true && cmake --build .

# Run the test application and collect code coverage data
# The html report can be found "build/coverage/index.html"
# Note thist runs basics test only (no connected umx devices)

# Basic tests
#./libum_test --gtest_filter=LibumTestBasic* && cmake --build . --target coverage
# Run all
#./libum_test && cmake --build . --target coverage
# uMp tests
./libum_test --gtest_filter=LibumTestBasic*:LibumTestUmp* && cmake --build . --target coverage

cd coverage
echo "Code coverage report: file://$(pwd)/index.html"
