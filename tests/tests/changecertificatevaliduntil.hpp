/*
 * Copyright (c) 2020-2023 Estonian Information System Authority
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

#include "pcsc-mock/pcsc-mock.hpp"

#include <ctime>
#include <string>

PcscMock::byte_vector::iterator findUTCDateTime(PcscMock::byte_vector::iterator first,
                                                PcscMock::byte_vector::iterator last)
{
    static const unsigned char UTC_DATETIME_TAG = 0x17;
    static const unsigned char LENGTH_TAG = 0x0d;

    for (; first != last; ++first) {
        if (*first == UTC_DATETIME_TAG && first + 1 != last && *(first + 1) == LENGTH_TAG) {
            return first;
        }
    }
    return last;
}

PcscMock::ApduScript replaceCertValidUntilYear(const PcscMock::ApduScript& script,
                                               const size_t certBytesStartOffset,
                                               const std::string& twoDigitYear)
{
    if (twoDigitYear.size() != 2) {
        throw std::invalid_argument("replaceCertValidUntilYear: twoDigitYear size must be 2, "
                                    "but is "
                                    + std::to_string(twoDigitYear.size()));
    }

    auto scriptCopy = script;
    auto& certBytes = scriptCopy[certBytesStartOffset].second;

    // UTCDateTime year example: 17:0d:31:32, 17 is tag, 0d is data lenght, 31:32 is '12'.

    // Find first occurrence of UTCDateTime tag, certificate valid from.
    auto dateTimeTag = findUTCDateTime(certBytes.begin(), certBytes.end());
    if (dateTimeTag == certBytes.end()) {
        // If not found, search next block; it is safe to assume offset + 1 exists in tests.
        certBytes = scriptCopy[certBytesStartOffset + 1].second;
        dateTimeTag = findUTCDateTime(certBytes.begin(), certBytes.end());
    }

    if (dateTimeTag == certBytes.end() || std::distance(dateTimeTag, certBytes.end()) < 20) {
        throw std::invalid_argument("replaceCertValidUntilYear: script bytes must contain "
                                    "UTCDateTime tag value 0x17 followed by at least 20 bytes");
    }

    // Find second occurrence of UTCDateTime tag, certificate valid until.
    dateTimeTag = findUTCDateTime(dateTimeTag + 1, certBytes.end());

    if (dateTimeTag == certBytes.end() || std::distance(dateTimeTag, certBytes.end()) < 5) {
        throw std::invalid_argument("replaceCertValidUntilYear: script bytes must contain "
                                    "second UTCDateTime tag value 0x17 followed by "
                                    "at least 5 bytes");
    }

    // Change the valid until year.
    *(dateTimeTag + 2) = static_cast<unsigned char>(twoDigitYear[0]);
    *(dateTimeTag + 3) = static_cast<unsigned char>(twoDigitYear[1]);

    return scriptCopy;
}

PcscMock::ApduScript replaceCertValidUntilTo2010(const PcscMock::ApduScript& script)
{
    return replaceCertValidUntilYear(script, 4, "10");
}

PcscMock::ApduScript replaceCertValidUntilToNextYear(const PcscMock::ApduScript& script)
{
    const auto t = std::time(nullptr);
    const auto now = std::localtime(&t);
    // UTCDateTime needs 2-digit year since 2000, tm_year is years since 1900, add +1 for next year
    const auto yearInt = now->tm_year + 1900 - 2000 + 1;
    const auto yearStr = std::to_string(yearInt);
    return replaceCertValidUntilYear(script, 4, yearStr);
}
