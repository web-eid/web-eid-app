#!/usr/bin/env zsh

set -e
set -u

export OPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
export GTest_ROOT=$(brew --prefix gtest)
export CMAKE_BUILD_PARALLEL_LEVEL=3
export MACOSX_DEPLOYMENT_TARGET=14.0

BUILD_TYPE=RelWithDebInfo
BUILD_DIR=build

if [[ ${1:-} == 'clean' ]]; then
  echo -n Cleaning...
  if [[ -d "./build" ]]; then
    cmake --build build --target clean
  fi
  echo DONE
fi

# For creating installers, you need to use signing certificates issued by Apple
# export SIGNCERT=<apple developer certificate name>

if (( ${+SIGNCERT} )); then
  cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -B ${BUILD_DIR} -DSIGNCERT="${SIGNCERT}" -S .
else
  cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -B ${BUILD_DIR} -S .
fi

cmake --build ${BUILD_DIR} --config ${BUILD_TYPE} # -- VERBOSE=1

ctest -V -C ${BUILD_TYPE} --test-dir build

# Uncomment in case SIGNCERT is set and you want to create installers
# To create web-eid installer for MacOS: build/src/app/web-eid*.dmg
# To create web-eid-webextension installer for firefox and chrome: build/src/app/web-eid*.pkg
# cmake --build ${BUILD_DIR} --config ${BUILD_TYPE} --target installer

# To create web-eid-webextension installer for safari build/src/mac/web-eid-safari_*.pkg
# cmake --build ${BUILD_DIR} --config ${BUILD_TYPE} --target installer-safari
