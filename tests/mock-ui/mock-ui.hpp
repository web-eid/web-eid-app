// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

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
        static QString pin {"1234"};
        return pin;
    }

public: // slots
    void onMultipleCertificatesReady(
        const QUrl&, const std::vector<EidCertificateAndPinInfo>& cardCertAndPin) override
    {
        emit accepted(cardCertAndPin[0]);
    }
    void onSingleCertificateReady(const QUrl&,
                                  const EidCertificateAndPinInfo& cardCertAndPin) override
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
