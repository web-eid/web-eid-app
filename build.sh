#!/bin/bash

set -e
set -u

export QT_QPA_PLATFORM=offscreen
export DEBIAN_FRONTEND=noninteractive

if [[ ${1:-} == 'clean' ]]; then
  echo -n Cleaning...
  cmake --build build --target clean
  rm -rf build
  echo DONE
fi

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build -S . 
cmake --build build --config RelWithDebInfo --target installer # -- VERBOSE=1
