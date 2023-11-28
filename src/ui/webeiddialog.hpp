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
#include "utils/qdisablecopymove.hpp"

#include <QCloseEvent>

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
class WebEidDialog final : public WebEidUI
{
    Q_OBJECT

public:
    enum class Page { WAITING, ALERT, SELECT_CERTIFICATE, PIN_INPUT, ABOUT };

    explicit WebEidDialog(QWidget* parent = nullptr);
    ~WebEidDialog() final;

    void showWaitingForCardPage(const CommandType commandType) final;
    QString getPin() final;

    static void showAboutPage();
    static void showFatalErrorPage();

    // slots
    void onSmartCardStatusUpdate(const RetriableError status) final;
    void onMultipleCertificatesReady(
        const QUrl& origin,
        const std::vector<CardCertificateAndPinInfo>& cardCertAndPinInfos) final;
    void onSingleCertificateReady(const QUrl& origin,
                                  const CardCertificateAndPinInfo& cardCertAndPinInfo) final;

    void onRetry(const RetriableError error) final;

    void onSigningCertificateMismatch() final;
    void onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                           const qint8 retriesLeft) final;
    void quit() final
    {
        closeUnconditionally = true;
        close();
    }

signals:
    void languageChange();

private:
    Q_DISABLE_COPY_MOVE(WebEidDialog)
    bool event(QEvent* event) final;
    void reject() final;

    void closeEvent(QCloseEvent* event) final
    {
        if (closeUnconditionally) {
            event->accept();
        } else {
            WebEidUI::closeEvent(event);
        }
    }

    void connectOkToCachePinAndEmitSelectedCertificate(const CardCertificateAndPinInfo& certAndPin);

    template <typename Text>
    void onRetryImpl(Text text);
    template <typename Text>
    void setTrText(QWidget* label, Text text) const;
    void
    setupCertificateAndPinInfo(const std::vector<CardCertificateAndPinInfo>& cardCertAndPinInfos);
    void setupPinPrompt(const PinInfo& pinInfo);
    void setupPinPadProgressBarAndEmitWait(const CardCertificateAndPinInfo& certAndPin);
    void setupPinInput(const CardCertificateAndPinInfo& certAndPin);
    template <typename Func>
    void setupOK(Func&& func, const std::function<QString()>& text = {}, bool enabled = false);
    void displayPinBlockedError();
    void displayFatalError(std::function<QString()> message);

    void showPinInputWarning(bool show);
    void resizeHeight();

    static QPixmap pixmap(QLatin1String name);
    static std::tuple<QString, QString, QPixmap>
    retriableErrorToTextTitleAndIcon(RetriableError error);

    class Private;
    Private* ui;

    CommandType currentCommand = CommandType::NONE;
    QString pin;
    bool closeUnconditionally = false;
};
