#!/bin/bash

set -e
set -u

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build -S .
cmake --build build # -- VERBOSE=1
