#!/bin/bash
# SPDX-FileCopyrightText: Estonian Information System Authority
# SPDX-License-Identifier: MIT

set -e
set -u

cd "$( dirname "$0" )/.."

find src/ tests/ -iname '*.hpp' -o -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i
