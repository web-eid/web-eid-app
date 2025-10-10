/*
 * Copyright (c) 2020-2024 Estonian Information System Authority
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
    void runCommandHandler(const std::vector<electronic_id::ElectronicID::ptr>& availableEids);
    void connectRetry(const ControllerChildThread* childThread);
    void createWindow() noexcept;
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
