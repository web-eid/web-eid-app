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

#include "commandhandler.hpp"

#include "ui.hpp"

#include "pcsc-cpp/pcsc-cpp.hpp"

#include <unordered_map>
#include <cstdint>

class ControllerChildThread;

/** Controller coordinates the execution flow and interaction between all other components. */
class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(CommandWithArgumentsPtr cmd) : QObject(nullptr), command(std::move(cmd)) {}

    const QVariantMap& result() const { return _result; }

signals:
    void quit();
    void statusUpdate(const electronic_id::AutoSelectFailed::Reason status);

public: // slots
    void run();

    // Called either directly from run() or from the monitor thread when the card is ready.
    void onCardReady(electronic_id::CardInfo::ptr cardInfo);

    // Called on reader and card events from monitor thread.
    void onReaderMonitorStatusUpdate(const electronic_id::AutoSelectFailed::Reason reason);

    // Called either directly from onDialogOK() or from the dialog when waiting for PIN-pad.
    void onCommandHandlerConfirm();

    // Called from CommandHandlerConfirm thread.
    void onCommandHandlerConfirmCompleted(const QVariantMap& result);

    // Called from the dialog when user chooses to retry errors that have occured in child threads.
    void onRetry();

    // User events from the dialog.
    void onDialogOK();
    void onDialogCancel();

    // Failure handler, reports the error and quits the application.
    void onCriticalFailure(const QString& error);

private:
    // Non-owning observing pointer.
    template <typename T>
    using observer_ptr = T*;

    void runCommandHandler();
    void startCommandExecution();
    void waitUntilSupportedCardSelected();
    void connectOkCancelWaitingForPinPad();
    void connectRetry(const ControllerChildThread* childThread);
    void disconnectRetry();
    void saveChildThreadPtrAndConnectFailureFinish(ControllerChildThread* childThread);
    void exit();
    void waitForChildThreads();
    void setCard(electronic_id::CardInfo::ptr cardInfo);
    CommandType commandType();

    CommandWithArgumentsPtr command;
    CommandHandler::ptr commandHandler = nullptr;
    std::unordered_map<uintptr_t, observer_ptr<ControllerChildThread>> childThreads;
    electronic_id::CardInfo::ptr cardInfo = nullptr;
    WebEidUI::ptr window = nullptr;
    QVariantMap _result;
    bool isInStdinMode = true;
};
