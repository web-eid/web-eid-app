#!/bin/bash
# SPDX-FileCopyrightText: Estonian Information System Authority
# SPDX-License-Identifier: MIT

set -e
set -u

export DEBIAN_FRONTEND=noninteractive

# Override for a debug build, e.g. BUILD_TYPE=Debug BUILD_SUFFIX=-debug ./build.sh
BUILD_TYPE=${BUILD_TYPE:-RelWithDebInfo}

# clean: reconfigure from an empty cache and rebuild everything
FRESH=
CLEAN_FIRST=
if [[ ${1:-} == 'clean' ]]; then
  FRESH=--fresh
  CLEAN_FIRST=--clean-first
fi

cmake --preset default $FRESH -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
cmake --build --preset default $CLEAN_FIRST # -- VERBOSE=1

if [[ ${1:-} == 'installer' ]]; then
  cmake --build --preset default --target installer
fi
