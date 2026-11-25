#!/usr/bin/env zsh
# SPDX-FileCopyrightText: Estonian Information System Authority
# SPDX-License-Identifier: MIT

set -e
set -u

export OPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
export GTest_ROOT=$(brew --prefix gtest)
export CMAKE_BUILD_PARALLEL_LEVEL=3

# Override for a debug build, e.g. BUILD_TYPE=Debug BUILD_SUFFIX=-debug ./build.zsh
BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo}

# clean: reconfigure from an empty cache and rebuild everything
FRESH=()
CLEAN_FIRST=()
if [[ ${1:-} == 'clean' ]]; then
  FRESH=(--fresh)
  CLEAN_FIRST=(--clean-first)
fi

# For creating installers, you need to use signing certificates issued by Apple
# export SIGNCERT=<apple developer certificate name>

if (( ${+SIGNCERT} )); then
  cmake --preset default $FRESH -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DSIGNCERT="${SIGNCERT}"
else
  cmake --preset default $FRESH -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
fi

cmake --build --preset default $CLEAN_FIRST # -- VERBOSE=1

ctest --preset default

# Uncomment in case SIGNCERT is set and you want to create installers
# To create web-eid installer for MacOS: build/Darwin/src/app/web-eid*.dmg
# To create web-eid-webextension installer for firefox and chrome: build/Darwin/src/app/web-eid*.pkg
# cmake --build --preset default --target installer

# To create web-eid-webextension installer for safari build/Darwin/src/mac/web-eid-safari_*.pkg
# cmake --build --preset default --target installer-safari
