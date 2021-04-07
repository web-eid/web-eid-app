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

#include "threads/readermonitorthread.hpp"
#include "threads/commandhandlerrunthread.hpp"
#include "threads/commandhandlerconfirmthread.hpp"

#include "utils.hpp"
#include "inputoutputmode.hpp"
#include "writeresponse.hpp"
#include "logging.hpp"

#include "magic_enum/magic_enum.hpp"

#include <QApplication>
#include <QScopedPointer>
#include <QVariantMap>

using namespace pcsc_cpp;
using namespace electronic_id;

namespace
{

// TODO: Should we use more detailed error codes? E.g. report input data error back to the website
// etc.
const QString RESP_TECH_ERROR = QStringLiteral("ERR_WEBEID_NATIVE_FATAL");
const QString RESP_USER_CANCEL = QStringLiteral("ERR_WEBEID_USER_CANCELLED");

QVariantMap makeErrorObject(const QString& errorCode, const QString& errorMessage)
{
    const auto errorBody = QVariantMap {
        {QStringLiteral("code"), errorCode},
        {QStringLiteral("message"), errorMessage},
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

        REQUIRE_NON_NULL(command)
        commandHandler = getCommandHandler(*command);

        startCommandExecution();

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::startCommandExecution()
{
    try {
        const auto availableCardInfos = availableSupportedCards();

        if (availableCardInfos.empty()) {
            waitUntilSupportedCardSelected();
        } else if (availableCardInfos.size() == 1) {
            const auto selectedCardInfo = availableCardInfos[0];
            qInfo() << "Reader" << selectedCardInfo->reader().name << "has supported card"
                    << selectedCardInfo->eid().name();

            onCardReady(selectedCardInfo);
        } else {
            qInfo() << "Readers" << availableCardInfos.size() << "has supported cards";
            onAvailableCards(availableCardInfos);
        }

    } catch (const AutoSelectFailed& failure) {
        qInfo() << "Card autoselect failed:"
                << std::string(magic_enum::enum_name(failure.reason()));
        waitUntilSupportedCardSelected();
    }
    CATCH_PCSC_CPP_RETRIABLE_ERRORS(warnAndWaitUntilSupportedCardSelected)
    CATCH_LIBELECTRONIC_ID_RETRIABLE_ERRORS(warnAndWaitUntilSupportedCardSelected)
}

void Controller::warnAndWaitUntilSupportedCardSelected(const RetriableError errorCode,
                                                       const std::exception& error)
{
    WARN_RETRIABLE_ERROR(std::string(commandType()), errorCode, error);
    waitUntilSupportedCardSelected();
}

void Controller::waitUntilSupportedCardSelected()
{
    // Reader monitor thread setup.
    ReaderMonitorThread* readerMonitorThread = new ReaderMonitorThread(this);
    connect(readerMonitorThread, &ReaderMonitorThread::statusUpdate, this,
            &Controller::onReaderMonitorStatusUpdate);
    connect(readerMonitorThread, &ReaderMonitorThread::cardReady, this, &Controller::onCardReady);
    connect(readerMonitorThread, &ReaderMonitorThread::availableCardInfos, this, &Controller::onAvailableCards);
    saveChildThreadPtrAndConnectFailureFinish(readerMonitorThread);

    // UI setup.
    window = WebEidUI::createAndShowDialog(CommandType::INSERT_CARD);
    connect(this, &Controller::statusUpdate, window.get(), &WebEidUI::onReaderMonitorStatusUpdate);
    connectOkCancelWaitingForPinPad();

    // Finally, start the thread to wait for card insertion after everything is wired up.
    readerMonitorThread->start();
}

void Controller::saveChildThreadPtrAndConnectFailureFinish(ControllerChildThread* childThread)
{
    REQUIRE_NON_NULL(childThread)
    // Save the thread pointer in child thread tracking map to request interruption and wait for
    // it to quit in waitForChildThreads().
    childThreads[uintptr_t(childThread)] = childThread;

    connect(childThread, &ControllerChildThread::failure, this, &Controller::onCriticalFailure);

    // When the thread is finished, remove the pointer from the tracking map and call deleteLater()
    // on it to free the thread object. Although the thread objects are freed through the Qt object
    // tree ownership system anyway, it is better to delete them immediately when they finish.
    connect(childThread, &ControllerChildThread::finished, this, [this, childThread]() {
        QScopedPointer<ControllerChildThread, QScopedPointerDeleteLater> deleteLater {childThread};

        const auto threadPtrAddress = uintptr_t(childThread);
        if (childThreads.count(threadPtrAddress) && childThreads[threadPtrAddress]) {
            childThread->wait();
            childThreads[threadPtrAddress] = nullptr;
        } else {
            qWarning() << "Controller child thread" << childThread
                       << "is missing or null in finish slot";
        }
    });
}

void Controller::connectOkCancelWaitingForPinPad()
{
    REQUIRE_NON_NULL(window)

    connect(window.get(), &WebEidUI::accepted, this, &Controller::onDialogOK);
    connect(window.get(), &WebEidUI::rejected, this, &Controller::onDialogCancel);
    connect(window.get(), &WebEidUI::waitingForPinPad, this, &Controller::onConfirmCommandHandler);
}

void Controller::onAvailableCards(const std::vector<electronic_id::CardInfo::ptr> &/*availableCards*/)
{

}

void Controller::onCardReady(const CardInfo::ptr& card)
{
    try {
        REQUIRE_NON_NULL(commandHandler)

        setCard(card);
        const auto protocol =
            card->eid().smartcard().protocol() == SmartCard::Protocol::T0 ? "T=0" : "T=1";
        qInfo() << "Using smart card protocol" << protocol;

        if (!window) {
            window = WebEidUI::createAndShowDialog(commandHandler->commandType());
            connectOkCancelWaitingForPinPad();
        } else {
            window->switchPage(commandHandler->commandType());
        }

        commandHandler->connectSignals(window.get());

        runCommandHandler();

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::runCommandHandler()
{
    try {
        CommandHandlerRunThread* commandHandlerRunThread =
            new CommandHandlerRunThread(this, *commandHandler, cardInfo);
        saveChildThreadPtrAndConnectFailureFinish(commandHandlerRunThread);
        connectRetry(commandHandlerRunThread);

        commandHandlerRunThread->start();

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::onReaderMonitorStatusUpdate(const RetriableError reason)
{
    emit statusUpdate(reason);
}

void Controller::onConfirmCommandHandler()
{
    try {
        CommandHandlerConfirmThread* commandHandlerConfirmThread =
            new CommandHandlerConfirmThread(this, *commandHandler, window.get());
        connect(commandHandlerConfirmThread, &CommandHandlerConfirmThread::completed, this,
                &Controller::onCommandHandlerConfirmCompleted);
        saveChildThreadPtrAndConnectFailureFinish(commandHandlerConfirmThread);
        connectRetry(commandHandlerConfirmThread);

        commandHandlerConfirmThread->start();

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::onCommandHandlerConfirmCompleted(const QVariantMap& res)
{
    try {
        _result = res;
        writeResponseToStdOut(isInStdinMode, res, commandHandler->commandType());
    } catch (const std::exception& error) {
        qCritical() << "Command" << std::string(commandType())
                    << "fatal error while writing response to stdout:" << error;
    }
    exit();
}

void Controller::onRetry()
{
    try {
        startCommandExecution();
    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::connectRetry(const ControllerChildThread* childThread)
{
    REQUIRE_NON_NULL(childThread)
    REQUIRE_NON_NULL(window)

    disconnect(window.get(), &WebEidUI::retry, nullptr, nullptr);

    connect(childThread, &ControllerChildThread::retry, window.get(), &WebEidUI::onRetry);
    connect(window.get(), &WebEidUI::retry, this, &Controller::onRetry);
}

void Controller::disconnectRetry()
{
    for (const auto& childThread : childThreads) {
        auto thread = childThread.second;
        if (thread) {
            disconnect(thread, &ControllerChildThread::retry, nullptr, nullptr);
        }
    }
}

void Controller::onDialogOK()
{
    if (commandHandler) {
        onConfirmCommandHandler();
    } else {
        // This should not happen, and when it does, OK should be equivalent to cancel.
        onDialogCancel();
    }
}

void Controller::onDialogCancel()
{
    qDebug() << "User cancelled";

    // Don't handle retry after user has cancelled.
    disconnectRetry();
    // Disconnect all signals from dialog to controller to avoid catching reject() twice.
    window.get()->disconnect(this);
    // Close the dialog.
    window.get()->close();

    writeResponseToStdOut(isInStdinMode,
                          makeErrorObject(RESP_USER_CANCEL, QStringLiteral("User cancelled")),
                          commandType());
    exit();
}

void Controller::onCriticalFailure(const QString& error)
{
    qCritical() << "Exiting due to command" << std::string(commandType())
                << "fatal error:" << error;
    writeResponseToStdOut(isInStdinMode, makeErrorObject(RESP_TECH_ERROR, error), commandType());

    if (window) {
        window->disconnect();
        window->close();
    }
    WebEidUI::showFatalError();

    exit();
}

void Controller::exit()
{
    waitForChildThreads();
    emit quit();
}

void Controller::waitForChildThreads()
{
    // Waiting for child thread must not happen in destructor.
    // See https://tombarta.wordpress.com/2008/07/10/gcc-pure-virtual-method-called/ for details.
    for (const auto& childThread : childThreads) {
        auto thread = childThread.second;
        if (thread) {
            thread->requestInterruption();
            // Waiting for PIN input on PIN pad may take a long time, call processEvents() so that
            // the UI doesn't freeze.
            while (thread->isRunning()) {
                thread->wait(100); // in milliseconds
                QCoreApplication::processEvents();
            }
        }
    }
}

void Controller::setCard(CardInfo::ptr card)
{
    REQUIRE_NON_NULL(card)
    this->cardInfo = card;
}

CommandType Controller::commandType()
{
    return commandHandler ? commandHandler->commandType() : CommandType(CommandType::INSERT_CARD);
}
