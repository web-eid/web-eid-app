#!/bin/bash

set -e
set -u

cd "$( dirname "$0" )/.."

rm -rf build/* lib/libelectronic-id/build/*
