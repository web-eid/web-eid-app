/*
 * Copyright (c) 2020 The Web eID Project
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

#include "qeid.hpp"

#include <QString>

enum class CertificateStatus { VALID, INVALID, NOT_YET_ACTIVE, EXPIRED };

Q_DECLARE_METATYPE(CertificateStatus)

struct CertificateInfo
{
    electronic_id::CertificateType type;
    QString icon;

    QString subject;
    QString issuer;
    QString effectiveDate;
    QString expiryDate;
};

Q_DECLARE_METATYPE(CertificateInfo)

struct PinInfo
{
    using PinMinMaxLength = std::pair<size_t, size_t>;
    using PinRetriesCount = std::pair<size_t, size_t>;

    PinMinMaxLength pinMinMaxLength;
    PinRetriesCount pinRetriesCount;
    bool readerHasPinPad;

    static constexpr int PIN_PAD_PIN_ENTRY_TIMEOUT = pcsc_cpp::PIN_PAD_PIN_ENTRY_TIMEOUT;
};

Q_DECLARE_METATYPE(PinInfo)

struct CertificateAndPinInfo
{
    CertificateStatus certStatus;
    CertificateInfo certInfo;
    PinInfo pinInfo;
};

Q_DECLARE_METATYPE(CertificateAndPinInfo)
