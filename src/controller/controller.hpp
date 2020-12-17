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

    // Called either directly from run() or from monitor thread when card is ready.
    void onCardReady(electronic_id::CardInfo::ptr cardInfo);

    // Reader and card events from monitor thread.
    void onReaderMonitorStatusUpdate(const electronic_id::AutoSelectFailed::Reason reason);

    // User events from dialog.
    void onDialogOK();
    void onDialogCancel();

    // Failure handler, reports the error and quits the application.
    void onCriticalFailure(const QString& error);

private:
    void startCommandExecution();
    void waitUntilSupportedCardSelected();
    void connectOkCancel();
    void setCard(electronic_id::CardInfo::ptr cardInfo);
    CommandType commandType();

    CommandWithArgumentsPtr command;
    CommandHandler::ptr commandHandler = nullptr;
    electronic_id::CardInfo::ptr cardInfo = nullptr;
    WebEidUI::ptr window = nullptr;
    QVariantMap _result;
    bool isInStdinMode = true;
};
