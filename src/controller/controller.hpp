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

#include "commandhandler.hpp"

#include "ui.hpp"

#include "pcsc-cpp/pcsc-cpp.hpp"

#include <unordered_map>
#include <cstdint>

class ControllerChildThread;
class CardEventMonitorThread;

/** Controller coordinates the execution flow and interaction between all other components. */
class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(CommandWithArgumentsPtr cmd) : QObject(nullptr), command(std::move(cmd)) {}

    const QVariantMap& result() const { return _result; }

signals:
    void quit();
    void statusUpdate(const RetriableError status);

public: // slots
    void run();

    // Called either directly from run() or from the monitor thread when cards are available.
    void onCardsAvailable(const std::vector<electronic_id::CardInfo::ptr>& availableCards);

    // Called when CommandHandlerRunThread finishes execution.
    void onCertificatesLoaded();

    // Called either directly from onDialogOK() or from the dialog when waiting for PIN-pad.
    void onConfirmCommandHandler(const CardCertificateAndPinInfo& cardCertAndPinInfo);

    // Called from CommandHandlerConfirm thread.
    void onCommandHandlerConfirmCompleted(const QVariantMap& result);

    // Called from the dialog when user chooses to retry errors that have occured in child threads.
    void onRetry();

    // User events from the dialog.
    void onDialogOK(const CardCertificateAndPinInfo& cardCertAndPinInfo);
    void onDialogCancel();

    // Failure handler, reports the error and quits the application.
    void onCriticalFailure(const QString& error);

private:
    // Non-owning observing pointer.
    template <typename T>
    using observer_ptr = T*;

    void startCommandExecution();
    void runCommandHandler(const std::vector<electronic_id::CardInfo::ptr>& availableCards);
    void connectOkCancelWaitingForPinPad();
    void connectRetry(const ControllerChildThread* childThread);
    void saveChildThreadPtrAndConnectFailureFinish(ControllerChildThread* childThread);
    void stopCardEventMonitorThread();
    void exit();
    void waitForChildThreads();
    CommandType commandType();

    CommandWithArgumentsPtr command;
    CommandHandler::ptr commandHandler = nullptr;
    std::unordered_map<uintptr_t, observer_ptr<ControllerChildThread>> childThreads;
    // Key of card event monitor thread in childThreads map.
    uintptr_t cardEventMonitorThreadKey = 0;
    WebEidUI::ptr window = nullptr;
    QVariantMap _result;
    bool isInStdinMode = true;
};
