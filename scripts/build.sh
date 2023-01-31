#!/bin/bash
set -e

BUILD_TYPE=${1:-Release}
echo $BUILD_TYPE

BUILD_DIR="build"
PROJECT_ROOT_PATH="$(pwd)/.."
BUILD_DIR_PATH="${PROJECT_ROOT_PATH}/${BUILD_DIR}"

mkdir -p ${BUILD_DIR_PATH}

cd ${BUILD_DIR_PATH}

cmake --fresh -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ${PROJECT_ROOT_PATH}
make clean
make
