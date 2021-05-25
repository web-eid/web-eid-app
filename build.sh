#!/bin/bash

set -e
set -u

PROJECT_ROOT="$(cd "$( dirname "$0" )"; pwd)"

# Verify that repository has been cloned with submodules

cd "$PROJECT_ROOT/lib/libelectronic-id"

[[ -e README.md ]] || { echo "FAIL: libelectronic-id submodule directory empty, did you 'git clone --recursive'?"; exit 1; }

# Build everything

mkdir -p "$PROJECT_ROOT/build"
cd "$PROJECT_ROOT/build"

BUILD_TYPE=RelWithDebInfo

cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE ..
cmake --build . --config $BUILD_TYPE # -- VERBOSE=1
