#!/bin/bash

set -e
set -u

export DEBIAN_FRONTEND=noninteractive

if [[ ${1:-} == 'clean' ]]; then
  echo -n Cleaning...
  if [[ -d "./build" ]]; then
    cmake --build build --target clean
  fi
  echo DONE
fi

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build -S .
cmake --build build --config RelWithDebInfo # -- VERBOSE=1

if [[ ${1:-} == 'installer' ]]; then
  cmake --build build --config RelWithDebInfo --target installer
fi
