// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "commands.hpp"
#include "certandpininfo.hpp"
#include "retriableerror.hpp"

#include "utils/observer_ptr.hpp"

#include <QDialog>

/**
 * The UI interface implemented in the ui and mock-ui projects.
 */
class WebEidUI : public QDialog
{
    Q_OBJECT

public:
    explicit WebEidUI(QWidget* parent = nullptr) : QDialog(parent) {}

    // Factory function that creates and shows the dialog that implements this interface.
    static observer_ptr<WebEidUI> createAndShowDialog(const CommandType command);

    static void showAboutPage();
    static void showFatalError();

    virtual void showWaitingForCardPage(const CommandType commandType) = 0;

    // getPin() is called from background threads and must be thread-safe.
    virtual QString getPin() = 0;

signals:
    void waitingForPinPad(const EidCertificateAndPinInfo& certAndPinInfo);
    void accepted(const EidCertificateAndPinInfo& certAndPinInfo);
    void retry();
    void failure(const QString& error);

public: // slots
    virtual void quit() = 0;
    virtual void onSmartCardStatusUpdate(const RetriableError status) = 0;
    virtual void
    onMultipleCertificatesReady(const QUrl& origin,
                                const std::vector<EidCertificateAndPinInfo>& certAndPinInfos) = 0;
    virtual void onSingleCertificateReady(const QUrl& origin,
                                          const EidCertificateAndPinInfo& certAndPinInfo) = 0;

    virtual void onRetry(const RetriableError error) = 0;

    virtual void onSigningCertificateMismatch() = 0;
    virtual void onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                                   const qint8 retriesLeft) = 0;

    virtual void forceClose() { close(); }
};
