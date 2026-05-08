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

#include "commandsession.hpp"
#include "responsesink.hpp"

#include <memory>

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

public: // slots
    void run() noexcept;

    // Called when CommandSession needs a fresh UI/session for the same command.
    void onRetry() noexcept;

    // Failure handler, reports the error and quits the application.
    void onCriticalFailure(const QString& error) noexcept;

private:
    void initializeResponseSink();
    void startCommandExecution();
    void createWindow(CommandType commandType);
    void onCommandSessionCompleted(const QVariantMap& result) noexcept;
    void onCommandSessionCancelled() noexcept;
    void disposeCommandSession() noexcept;
    void disposeUI() noexcept;
    void exit() noexcept;
    CommandType commandType() const noexcept;
    void writeResult(const QVariantMap& result, CommandType resultCommandType);

    CommandWithArgumentsPtr command;
    std::unique_ptr<ResponseSink> responseSink;
    std::unique_ptr<CommandSession> commandSession;
    // As the Qt::WA_DeleteOnClose flag is set, the dialog is deleted automatically.
    observer_ptr<WebEidUI> window = nullptr;
    QVariantMap _result;
};
