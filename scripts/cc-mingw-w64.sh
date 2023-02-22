#!/bin/bash
# This script cross compiles binaries for Windows by minwg-w64
set -e

PROJECT_ROOT_PATH="$(dirname ${0})/.."

function crossCompile() {
    TYPE=$1
    TOOLCHAIN_FILE=$2
    ARC=$3

    # Validate arduments
    if [[ ($TYPE == 'Release' || $TYPE == 'Debug') && ($ARC == 'x64' || $ARC == 'i686') ]]
    then
        BUILD_DIR="build-mingw-${ARC}-${TYPE}"
    else
        #invalid argument
        return 1;
    fi

    CURRENT_PATH=$(pwd);

    # Clear old tailings
    cd ${PROJECT_ROOT_PATH}
    rm -rf $BUILD_DIR
    mkdir $BUILD_DIR
    cd $BUILD_DIR

    cmake -DCMAKE_BUILD_TYPE=${TYPE} \
          -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
           ..
    make clean
    make

    cd "${CURRENT_PATH}"

    return 0
}

# Set the type for this compile.
BUILD_TYPE=${1:-Release}
echo $BUILD_TYPE

CMAKE_CONFIG_FOLDER="${PROJECT_ROOT_PATH}/cmake"

# Crosscompile for X64 (64-bit)
X64_TOOLCHAIN_FILE="${CMAKE_CONFIG_FOLDER}/mingw-w64-x86_64.cmake"
crossCompile ${BUILD_TYPE} ${X64_TOOLCHAIN_FILE} x64

# Crosscompile for i686 (32-bit)
i686_TOOLCHAIN_FILE="${CMAKE_CONFIG_FOLDER}/mingw-w64-i686.cmake"
crossCompile ${BUILD_TYPE} ${i686_TOOLCHAIN_FILE} i686
