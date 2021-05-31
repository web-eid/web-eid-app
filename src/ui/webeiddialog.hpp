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

#include "ui.hpp"

class CertificateListWidget;
class QLabel;

// clang-format off
/**
 * The WebEidDialog class contains all UI elements of the web-eid application.
 *
 * The dialog consists of OK/Cancel buttons and a QStackedWidget with the following pages:
 * - waiting
 * - message,
 * - select certificate,
 * - pin input.
 */
// clang-format on
class WebEidDialog : public WebEidUI
{
    Q_OBJECT

public:
    enum class Page { WAITING, ALERT, SELECT_CERTIFICATE, PIN_INPUT };

    explicit WebEidDialog(QWidget* parent = nullptr);
    ~WebEidDialog() override;

    void showWaitingForCardPage(const CommandType commandType) override;
    QString getPin() override;

public: // slots
    void onSmartCardStatusUpdate(const RetriableError status) override;
    void onMultipleCertificatesReady(
        const QUrl& origin,
        const std::vector<CardCertificateAndPinInfo>& cardCertAndPinInfos) override;
    void onSingleCertificateReady(const QUrl& origin,
                                  const CardCertificateAndPinInfo& cardCertAndPinInfo) override;

    void onRetry(const RetriableError error) override;

    void onCertificateNotFound(const QString& subjectOfUserCertFromArgs) override;
    void onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                           const quint8 retriesLeft) override;

private:
    bool event(QEvent* event) override;
    void reject() override;

    void connectOkToCachePinAndEmitSelectedCertificate(const CardCertificateAndPinInfo& certAndPin);

    void onRetryImpl(const QString& error);

    void
    setupCertificateAndPinInfo(const std::vector<CardCertificateAndPinInfo>& cardCertAndPinInfos);
    void setupPinPadProgressBarAndEmitWait(const CardCertificateAndPinInfo& certAndPin);
    void setupPinInputValidator(const PinInfo::PinMinMaxLength& pinInfo);

    void setupOK(const std::function<void()>& func, const QString& label = {},
                 bool enabled = false);
    void displayPinRetriesRemaining(PinInfo::PinRetriesCount pinRetriesCount);
    void displayPinBlockedError();

    void resizeHeight();

    std::tuple<QString, QString, QString>
    retriableErrorToTextTitleAndIcon(const RetriableError error);

    class Private;
    Private* ui;

    CommandType currentCommand = CommandType::NONE;
    QString pin;
};
