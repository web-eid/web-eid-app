// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "commandhandler.hpp"

class ControllerChildThread;

/** Controller coordinates the execution flow and interaction between all other components. */
class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(CommandWithArgumentsPtr cmd) : command(std::move(cmd)) {}

    const QVariantMap& result() const { return _result; }

signals:
    void quit();
    void retry(const RetriableError error);
    void statusUpdate(RetriableError status);
    void stopCardEventMonitorThread();

public: // slots
    void run() noexcept;

    // Called either directly from run() or from the monitor thread when cards are available.
    void
    onCardsAvailable(const std::vector<electronic_id::ElectronicID::ptr>& availableEids) noexcept;

    // Called when CommandHandlerRunThread finishes execution.
    void onCertificatesLoaded() noexcept;

    // Called either directly from onDialogOK().
    void onConfirmCommandHandler(const EidCertificateAndPinInfo& certAndPinInfo) noexcept;

    // Called from CommandHandlerConfirm thread.
    void onCommandHandlerConfirmCompleted(const QVariantMap& result) noexcept;

    // Called from the dialog when user chooses to retry errors that have occured in child threads.
    void onRetry() noexcept;

    // User events from the dialog.
    void onDialogOK(const EidCertificateAndPinInfo& certAndPinInfo) noexcept;
    void onDialogCancel() noexcept;

    // Failure handler, reports the error and quits the application.
    void onCriticalFailure(const QString& error) noexcept;

private:
    void startCommandExecution();
    void connectRetry(const ControllerChildThread* childThread) const;
    void createWindow();
    void disposeUI() noexcept;
    void exit() noexcept;
    void waitForChildThreads() noexcept;
    CommandType commandType() const noexcept;

    CommandWithArgumentsPtr command;
    CommandHandler::ptr commandHandler;
    // As the Qt::WA_DeleteOnClose flag is set, the dialog is deleted automatically.
    observer_ptr<WebEidUI> window = nullptr;
    QVariantMap _result;
    bool isInStdinMode = true;
};
