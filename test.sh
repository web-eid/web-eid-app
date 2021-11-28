#!/bin/bash

set -e
set -u

PROJECT_ROOT="$(cd "$( dirname "$0" )"; pwd)"

# Build project

$PROJECT_ROOT/build.sh

# Run project tests

export QT_QPA_PLATFORM='offscreen' # needed for running headless tests

ctest -V --test-dir build
