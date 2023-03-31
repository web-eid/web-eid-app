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

#include "ui.hpp"

class MockUI : public WebEidUI
{
    Q_OBJECT

public:
    explicit MockUI(QWidget* parent = nullptr) : WebEidUI(parent) {}

    void showWaitingForCardPage(const CommandType) override {}

    QString getPin() override
    {
        static QString pin {"0090"};
        return pin;
    }

public: // slots
    void onMultipleCertificatesReady(
        const QUrl&, const std::vector<CardCertificateAndPinInfo>& cardCertAndPin) override
    {
        emit accepted(cardCertAndPin[0]);
    }
    void onSingleCertificateReady(const QUrl&,
                                  const CardCertificateAndPinInfo& cardCertAndPin) override
    {
        emit accepted(cardCertAndPin);
    }

    void onSigningCertificateMismatch() override {}

    void onRetry(const RetriableError) override { emit rejected(); }

    void onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status, const qint8) override {}

    void onSmartCardStatusUpdate(const RetriableError) override
    {
        emit rejected();
        // Schedule invoking Controller::exit().
        emit destroyed();
    }

    void quit() final
    {
        // Schedule invoking Controller::exit().
        emit destroyed();
    }
};
