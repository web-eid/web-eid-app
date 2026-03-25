// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "pcsc-cpp/pcsc-cpp-utils.hpp"

#define REQUIRE_NOT_EMPTY_CONTAINS_NON_NULL_PTRS(vec)                                              \
    if (vec.empty()) {                                                                             \
        THROW(std::logic_error, #vec " is empty");                                                 \
    }                                                                                              \
    for (const auto& ptr : vec) {                                                                  \
        REQUIRE_NON_NULL(ptr)                                                                      \
    }
