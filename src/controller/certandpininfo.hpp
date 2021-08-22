/*
 * Copyright (c) 2020-2021 Estonian Information System Authority
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

#include <QSslCertificate>

struct CertificateInfo
{
    electronic_id::CertificateType type = electronic_id::CertificateType::NONE;

    QString subject;
    QString issuer;
    QString effectiveDate;
    QString expiryDate;
};

struct PinInfo
{
    using PinMinMaxLength = std::pair<size_t, size_t>;
    using PinRetriesCount = std::pair<size_t, size_t>;

    PinMinMaxLength pinMinMaxLength = {0, 0};
    PinRetriesCount pinRetriesCount = {0, 0};
    bool readerHasPinPad = false;
    bool pinIsBlocked = false;

    static constexpr int PIN_PAD_PIN_ENTRY_TIMEOUT = pcsc_cpp::PIN_PAD_PIN_ENTRY_TIMEOUT;
};

struct CardCertificateAndPinInfo
{
    electronic_id::CardInfo::ptr cardInfo;
    QByteArray certificateBytesInDer;
    QSslCertificate certificate;
    CertificateInfo certInfo;
    PinInfo pinInfo;
};

Q_DECLARE_METATYPE(CardCertificateAndPinInfo)
