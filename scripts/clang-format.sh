#!/bin/bash

set -e
set -u

cd "$( dirname "$0" )/.."

find src/ tests/ -iname '*.hpp' -o -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i
