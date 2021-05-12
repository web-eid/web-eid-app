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

#include <optional>
#include <utility>

class CertificateListWidget;
class QLabel;
class QLineEdit;

namespace Ui
{
class WebEidDialog;
}

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
    enum class Page { WAITING, MESSAGE, SELECT_CERTIFICATE, PININPUT };

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
    void showPage(const WebEidDialog::Page page);

    void connectOkToEmitSelectedCertificate(CertificateListWidget* certificateWidget);
    void connectOkToCachePinAndEmitSelectedCertificate(const CardCertificateAndPinInfo& certAndPin);

    void onRetryImpl(const QString& error);

    void setupPinPadProgressBarAndEmitWait();
    void setupPinInputValidator(const PinInfo::PinMinMaxLength& pinInfo);

    void startPinTimeoutProgressBar();
    void enableAndShowOK();
    void disableOKUntilCertificateSelected(const CertificateListWidget* certificateWidget);
    void displayPinRetriesRemaining(PinInfo::PinRetriesCount pinRetriesCount);
    void displayPinBlockedError();

    void resizeHeight();

    std::pair<QLabel*, CertificateListWidget*> originLabelAndCertificateListOnPage();

    std::tuple<QString, QString, QString>
    retriableErrorToTextTitleAndIcon(const RetriableError error);

    class Private;
    Private* ui;

    CommandType currentCommand = CommandType::NONE;
    std::optional<bool> readerHasPinPad;
    QString pin;
};
