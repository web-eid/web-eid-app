#!/bin/bash
# SPDX-FileCopyrightText: Estonian Information System Authority
# SPDX-License-Identifier: MIT

set -e
set -u

cd "$( dirname "$0" )/.."

rm -rf build/* lib/libelectronic-id/build/*
