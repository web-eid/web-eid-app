#!/bin/bash

set -e
set -u

cd "$( dirname "$0" )/.."

[[ $(git status --porcelain) == "" ]] || { echo "FAIL: Git status dirty, see 'git status' output:"; git status; exit 1; }

