#!/bin/bash

set -e
set -u

PROJECT_ROOT="$(cd "$( dirname "$0" )"; pwd)"

# Build project

$PROJECT_ROOT/build.sh

# Run project tests

ctest -V --test-dir build
