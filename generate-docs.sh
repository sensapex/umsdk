#!/bin/bash
set -e

# Get the current sdk version number
UMLIB_VER=$(grep 'define LIBUM_VERSION_STR' src/libum.c | awk '{ print $3 }' | sed 's/"//g')
echo -n "${UMLIB_VER}" > doxygen-umsdk-version.txt

# Use it to update PROJECT_NUMBER runtime
(cat Doxyfile; echo PROJECT_NUMBER="${UMLIB_VER}" ) | doxygen -
