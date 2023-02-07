#!/bin/bash
set -e

BUILD_TYPE=${1:-Release}
echo $BUILD_TYPE

BUILD_DIR="build"

PROJECT_ROOT_PATH="$(dirname ${0})/.."
BUILD_DIR_PATH="${PROJECT_ROOT_PATH}/${BUILD_DIR}-${BUILD_TYPE}"

mkdir -p ${BUILD_DIR_PATH}

cd ${BUILD_DIR_PATH}
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ..
make clean
make
