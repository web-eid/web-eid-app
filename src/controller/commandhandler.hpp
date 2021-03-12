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

/** Interface for command handlers that implement the actual work the application does, like get
 * certificate, authenticate and sign. */
class CommandHandler : public QObject
{
    Q_OBJECT

public:
    using ptr = std::unique_ptr<CommandHandler>;

    virtual void run(electronic_id::CardInfo::ptr cardInfo) = 0;
    virtual void connectSignals(const WebEidUI* window) = 0;
    virtual QVariantMap onConfirm(WebEidUI* window) = 0;

    CommandType commandType() const { return command.first; }

protected:
    CommandHandler(const CommandWithArguments& cmd) : command(cmd) {}
    CommandWithArguments command;
};

CommandHandler::ptr getCommandHandler(const CommandWithArguments& cmd);

/** Exception that signals expected invalid PIN entry errors. */
class CommandHandlerVerifyPinFailed : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/** Exception that signals other errors that allow the user to retry the action. */
class CommandHandlerRetriableError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};
