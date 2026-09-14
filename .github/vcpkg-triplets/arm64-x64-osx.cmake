# SPDX-FileCopyrightText: Estonian Information System Authority
# SPDX-License-Identifier: MIT
#
# Based on vcpkg's arm64-osx triplet (MIT), https://github.com/microsoft/vcpkg
# Differs in building a universal binary and pinning the 14.0 deployment target.

set(VCPKG_TARGET_ARCHITECTURE arm64 x86_64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES arm64 x86_64)
set(VCPKG_OSX_DEPLOYMENT_TARGET 14.0)
