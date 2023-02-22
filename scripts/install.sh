#!/bin/bash
set -e
BUILD_DIR="build-Release"
PROJECT_ROOT_PATH="$(dirname ${0})/.."

BUILD_DIR_PATH="${PROJECT_ROOT_PATH}/${BUILD_DIR}"

cd ${BUILD_DIR_PATH}
sudo make install
