#!/bin/bash
# SPDX-FileCopyrightText: Estonian Information System Authority
# SPDX-License-Identifier: MIT

set -e
set -u

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -B build -S .
cmake --build build # -- VERBOSE=1
