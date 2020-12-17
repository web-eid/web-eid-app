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

#include "controller.hpp"
#include "readermonitorthread.hpp"
#include "utils.hpp"
#include "inputoutputmode.hpp"
#include "writeresponse.hpp"
#include "logging.hpp"

#include "magic_enum/magic_enum.hpp"

#include <QApplication>
#include <QVariantMap>

using namespace pcsc_cpp;
using namespace electronic_id;

namespace
{

const QString RESP_TECH_ERROR = QStringLiteral("ERR_WEBEID_NATIVE_FATAL");
const QString RESP_USER_CANCEL = QStringLiteral("ERR_WEBEID_USER_CANCELLED");

QVariantMap makeErrorObject(const QString& errorCode, const QString& errorMessage)
{
    const auto errorBody = QVariantMap {
        {QStringLiteral("code"), errorCode}, {QStringLiteral("message"), errorMessage},
        // TODO:
        // {"location", "FILE:LINE:function"},
        // {"nativeException", "sometype:message:FILE:LINE:function"},
    };
    return {{QStringLiteral("error"), errorBody}};
}

} // namespace

void Controller::run()
{
    // If a command is passed, the application is in command-line mode, else in stdin/stdout mode.
    const bool isInCommandLineMode = bool(command);
    isInStdinMode = !isInCommandLineMode;

    qInfo() << "web-eid app" << qApp->applicationVersion() << "running in"
            << (isInStdinMode ? "stdin/stdout" : "command-line") << "mode";

    try {
        if (isInStdinMode) {

            // In stdin/stdout mode we first output the version as required by the WebExtension
            // and then wait for the actual command.
            writeResponseToStdOut(isInStdinMode,
                                  {{QStringLiteral("version"), qApp->applicationVersion()}},
                                  "get-version");

            command = readCommandFromStdin();
        }

        requireNonNull(command, "command", "run");
        commandHandler = getCommandHandler(*command);

        startCommandExecution();

        // TODO:
        // 1. catch PCSC, libelectronic-id and controller errors separately
        // 2. try to discern retriable errors from fatal errors
        // 3. pass error code and message back to caller in stdin mode
    } catch (const std::exception& error) {
        // FIXME: Pass errors back up to caller in stdin mode.
        onCriticalFailure(error.what());
    }
}

void Controller::startCommandExecution()
{
    try {
        const auto selectedCardInfo = autoSelectSupportedCard();

        if (selectedCardInfo) {
            qInfo() << "Reader" << selectedCardInfo->reader().name << "has supported card"
                    << selectedCardInfo->eid().name();

            onCardReady(selectedCardInfo);
        } else {
            waitUntilSupportedCardSelected();
        }

    } catch (const AutoSelectFailed& failure) {
        qInfo() << "catch AutoSelectFailed:"
                << std::string(magic_enum::enum_name(failure.reason()));
        waitUntilSupportedCardSelected();

    } catch (const ScardError& error) {
        // TODO: some ScardErrors may be fatal, exit in this case. Investigate, think, discuss.
        qWarning() << "catch ScardError:" << error;
        waitUntilSupportedCardSelected();
    }
}

void Controller::waitUntilSupportedCardSelected()
{
    // Reader monitor thread setup.

    // Controller takes ownership of the thread object and frees it through the Qt object tree
    // ownership system. Regardless of that we call deleteLater() on the thread manually to free the
    // thread object immediately when it is finished. We also need access to the thread to request
    // interruption and wait for it to quit.
    ReaderMonitorThread* readerMonitorThread = new ReaderMonitorThread(this);
    qDebug() << "Reader monitor thread created";

    connect(readerMonitorThread, &ReaderMonitorThread::statusUpdate, this,
            &Controller::onReaderMonitorStatusUpdate);
    connect(readerMonitorThread, &ReaderMonitorThread::cardReady, this, &Controller::onCardReady);
    connect(readerMonitorThread, &ReaderMonitorThread::readerMonitorFailure, this,
            &Controller::onCriticalFailure);

    // UI setup.
    window = WebEidUI::createAndShowDialog(CommandType::INSERT_CARD);

    connect(this, &Controller::statusUpdate, window.get(), &WebEidUI::onReaderMonitorStatusUpdate);

    connectOkCancel();

    // Start the thread after everything is wired up.

    readerMonitorThread->start();
}

void Controller::connectOkCancel()
{
    requireNonNull(window, "window", "connectOkCancel");

    connect(window.get(), &WebEidUI::accepted, this, &Controller::onDialogOK);
    connect(window.get(), &WebEidUI::rejected, this, &Controller::onDialogCancel);
}

void Controller::onCardReady(CardInfo::ptr card)
{
    try {
        setCard(card);
        const auto protocol =
            card->eid().smartcard().protocol() == SmartCard::Protocol::T0 ? "T=0" : "T=1";
        qInfo() << "Using smart card protocol" << protocol;

        requireNonNull(commandHandler, "commandHandler", "onCardReady");

        if (!window) {
            window = WebEidUI::createAndShowDialog(commandHandler->commandType());
            connectOkCancel();
        } else {
            window->switchPage(commandHandler->commandType());
        }

        commandHandler->connectSignals(window.get());
        commandHandler->run(cardInfo);

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::onReaderMonitorStatusUpdate(const AutoSelectFailed::Reason reason)
{
    emit statusUpdate(reason);
}

void Controller::onDialogOK()
{
    if (commandHandler) { // Do nothing unless command handler has been initialized.
        const auto commandType = std::string(commandHandler->commandType());
        try {
            _result = commandHandler->onConfirm(window.get());
            qInfo() << "Command" << commandType << "completed successfully";

            writeResponseToStdOut(isInStdinMode, _result, commandType);
            emit quit();

        } catch (const CommandHandlerRetriableError& error) {
            qWarning() << "Command" << commandType << "retriable error:" << error;
            return; // Don't exit, continue on retriable error.
        } catch (const std::exception& error) {
            qCritical() << "Command" << commandType << "fatal error:";
            onCriticalFailure(error.what());
        }

    } else {
        // This should not happen, and when it does, OK should be equivalent to cancel.
        onDialogCancel();
    }
}

void Controller::onDialogCancel()
{
    qDebug() << "User cancelled";
    writeResponseToStdOut(isInStdinMode,
                          makeErrorObject(RESP_USER_CANCEL, QStringLiteral("User cancelled")),
                          commandType());
    emit quit();
}

void Controller::onCriticalFailure(const QString& error)
{
    qCritical() << error;
    // TODO: use proper error codes instead of TECHNICAL_ERROR
    writeResponseToStdOut(isInStdinMode, makeErrorObject(RESP_TECH_ERROR, error), commandType());
    emit quit();
}

void Controller::setCard(CardInfo::ptr card)
{
    requireNonNull(card, "card", "setCard");
    this->cardInfo = card;
}

CommandType Controller::commandType()
{
    return commandHandler ? commandHandler->commandType() : CommandType(CommandType::INSERT_CARD);
}
