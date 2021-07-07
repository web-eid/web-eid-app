/*
 * Copyright (c) Estonian Information System Authority
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

#include "certificatereader.hpp"

class Authenticate : public CertificateReader
{
    Q_OBJECT

public:
    explicit Authenticate(const CommandWithArguments& cmd);

    void connectSignals(const WebEidUI* window) override;
    QVariantMap onConfirm(WebEidUI* window,
                          const CardCertificateAndPinInfo& cardCertAndPin) override;

signals:
    void verifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                         const quint8 retriesLeft);

private:
    void validateAndStoreOriginCertificate(const QVariantMap& args);

    QString nonce;
    QSslCertificate originCertificate;
};
