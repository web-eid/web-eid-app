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
#include "certandpininfo.hpp"
#include "qeid.hpp"

#include <optional>

class QLabel;
class QLineEdit;
class QProgressBar;

namespace Ui
{
class WebEidDialog;
}

// clang-format off
/**
 * The WebEidDialog class contains all UI elements of the web-eid application.
 *
 * The dialog consists of OK/Cancel buttons and a QStackedWidget with the following pages:
 * - connect smart card,
 * - select certificate,
 * - authenticate,
 * - sign.
 */
// clang-format on
class WebEidDialog : public WebEidUI
{
    Q_OBJECT

public:
    explicit WebEidDialog(QWidget* parent = nullptr);
    ~WebEidDialog() override;

    void switchPage(const CommandType commandType) override;
    QString getPin() override;

public: // slots
    void onReaderMonitorStatusUpdate(const RetriableError status) override;
    void onCertificateReady(const QUrl& origin, const CertificateStatus certStatus,
                            const CertificateInfo& certInfo, const PinInfo& pinInfo) override;
    void onDocumentHashReady(const QString& docHash) override;
    void onSigningCertificateHashMismatch(const QString& subjectOfUserCertFromArgs) override;
    void onRetry(const RetriableError error) override;
    void onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                           const quint8 retriesLeft) override;

private:
    void onOkButtonClicked(); // slot

    void makeOkButtonDefaultAndconnectSignals();
    void setupPinInputValidator(const PinInfo::PinMinMaxLength& pinMinMaxLenght);
    void startPinTimeoutProgressBar();
    std::tuple<QLabel*, QLabel*, QLabel*, QLabel*> certificateLabelsOnPage();
    QLabel* pinErrorLabelOnPage();
    QLabel* pinTitleLabelOnPage();
    QLineEdit* pinInputOnPage();
    QProgressBar* pinEntryTimeoutProgressBarOnPage();
    void displayFatalError(QLabel* label, const QString& message);
    void hidePinAndDocHashWidgets();
    void onRetryImpl(const QString& error);
    QString retriableErrorToString(const RetriableError error);

    Ui::WebEidDialog* ui;
    QPushButton* okButton; // non-owning pointer
    CommandType currentCommand = CommandType::NONE;
    int lineHeight = -1;
    std::optional<bool> readerHasPinPad;
    QString pin;
};
