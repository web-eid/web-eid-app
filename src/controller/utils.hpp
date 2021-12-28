/*
 * Copyright (c) 2020-2022 Estonian Information System Authority
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "pcsc-cpp/pcsc-cpp-utils.hpp"

#define REQUIRE_NON_NULL(val)                                                                      \
    if (!val) {                                                                                    \
        throw std::logic_error("Null " + std::string(#val) + " in "                                \
                               + pcsc_cpp::removeAbsolutePathPrefix(__FILE__) + ':'                \
                               + std::to_string(__LINE__) + ':' + __func__);                       \
    }

#define REQUIRE_NOT_EMPTY_CONTAINS_NON_NULL_PTRS(vec)                                              \
    if (vec.empty()) {                                                                             \
        throw std::logic_error(std::string(#vec) + " is empty in "                                 \
                               + pcsc_cpp::removeAbsolutePathPrefix(__FILE__) + ':'                \
                               + std::to_string(__LINE__) + ':' + __func__);                       \
    }                                                                                              \
    for (const auto& ptr : vec) {                                                                  \
        REQUIRE_NON_NULL(ptr)                                                                      \
    }
