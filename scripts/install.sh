#!/bin/bash
set -e
BUILD_DIR="build"
PROJECT_ROOT_PATH="$(pwd)/.."
BUILD_DIR_PATH="${PROJECT_ROOT_PATH}/${BUILD_DIR}"

cd ${BUILD_DIR_PATH}
sudo make install
