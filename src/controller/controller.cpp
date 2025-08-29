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

#include "controller.hpp"

#include "threads/cardeventmonitorthread.hpp"
#include "threads/commandhandlerconfirmthread.hpp"
#include "threads/commandhandlerrunthread.hpp"
#include "threads/waitforcardthread.hpp"

#include "utils/utils.hpp"

#include "inputoutputmode.hpp"
#include "writeresponse.hpp"

#include <QApplication>

using namespace pcsc_cpp;
using namespace electronic_id;

namespace
{

const QString RESP_TECH_ERROR = QStringLiteral("ERR_WEBEID_NATIVE_FATAL");
const QString RESP_USER_CANCEL = QStringLiteral("ERR_WEBEID_USER_CANCELLED");

QVariantMap makeErrorObject(const QString& errorCode, const QString& errorMessage)
{
    return {{QStringLiteral("error"),
             QVariantMap {
                 {QStringLiteral("code"), errorCode},
                 {QStringLiteral("message"), errorMessage},
             }}};
}

} // namespace

void Controller::run()
{
    // If a command is passed, the application is in command-line mode, else in stdin/stdout mode.
    const bool isInCommandLineMode = bool(command);
    isInStdinMode = !isInCommandLineMode;

    qInfo() << qApp->applicationName() << "app" << qApp->applicationVersion() << "running in"
            << (isInStdinMode ? "stdin/stdout" : "command-line") << "mode";

    try {
        // TODO: cut out stdin mode separate class to avoid bugs in safari mode
        if (isInStdinMode) {
            // In stdin/stdout mode we first output the version as required by the WebExtension
            // and then wait for the actual command.
            writeResponseToStdOut(isInStdinMode,
                                  {{QStringLiteral("version"), qApp->applicationVersion()}},
                                  "get-version");

            command = readCommandFromStdin();
        }

        REQUIRE_NON_NULL(command)
        switch (command->first) {
        case CommandType::ABOUT:
            WebEidUI::showAboutPage();
            return;
        case CommandType::QUIT:
            // If quit is requested, respond with empty JSON object and quit immediately.
            qInfo() << "Quit requested, exiting";
            writeResponseToStdOut(true, {}, "quit");
            emit quit();
            return;
        default:
            break;
        }

        commandHandler = getCommandHandler(*command);

        startCommandExecution();

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::startCommandExecution()
{
    REQUIRE_NON_NULL(commandHandler)

    // Reader monitor thread setup.
    auto* waitForCardThread = new WaitForCardThread(this);
    connect(this, &Controller::stopCardEventMonitorThread, waitForCardThread,
            &WaitForCardThread::requestInterruption);
    connect(waitForCardThread, &WaitForCardThread::statusUpdate, this, &Controller::statusUpdate);
    connect(waitForCardThread, &WaitForCardThread::cardsAvailable, this,
            &Controller::onCardsAvailable);
    connect(waitForCardThread, &ControllerChildThread::failure, this,
            &Controller::onCriticalFailure);

    // UI setup.
    window = WebEidUI::createAndShowDialog(commandHandler->commandType());
    connect(this, &Controller::statusUpdate, window, &WebEidUI::onSmartCardStatusUpdate);
    connectOkCancelWaitingForPinPad();

    // Finally, start the thread to wait for card insertion after everything is wired up.
    waitForCardThread->start();
}

void Controller::connectOkCancelWaitingForPinPad()
{
    REQUIRE_NON_NULL(window)

    connect(window, &WebEidUI::accepted, this, &Controller::onDialogOK);
    connect(window, &WebEidUI::rejected, this, &Controller::onDialogCancel);
    connect(window, &WebEidUI::failure, this, &Controller::onCriticalFailure);
    connect(window, &WebEidUI::waitingForPinPad, this, &Controller::onConfirmCommandHandler);
}

void Controller::onCardsAvailable(
    const std::vector<electronic_id::ElectronicID::ptr>& availableEids)
{
    try {
        REQUIRE_NON_NULL(commandHandler)
        REQUIRE_NON_NULL(window)
        REQUIRE_NOT_EMPTY_CONTAINS_NON_NULL_PTRS(availableEids)

        for (const auto& card : availableEids) {
            const auto protocol =
                card->smartcard().protocol() == SmartCard::Protocol::T0 ? "T=0" : "T=1";
            qInfo() << "Card" << card->name() << "in reader" << card->smartcard().readerName()
                    << "using protocol" << protocol;
        }

        window->showWaitingForCardPage(commandHandler->commandType());

        commandHandler->connectSignals(window);

        runCommandHandler(availableEids);

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::runCommandHandler(const std::vector<ElectronicID::ptr>& availableEids)
{
    try {
        auto* commandHandlerRunThread =
            new CommandHandlerRunThread(this, *commandHandler, availableEids);
        connect(commandHandlerRunThread, &ControllerChildThread::failure, this,
                &Controller::onCriticalFailure);
        connectRetry(commandHandlerRunThread);

        // When the command handler run thread retrieves certificates successfully, call
        // onCertificatesLoaded() that starts card event monitoring while user enters the PIN.
        connect(commandHandler.get(), &CommandHandler::singleCertificateReady, this,
                &Controller::onCertificatesLoaded);
        connect(commandHandler.get(), &CommandHandler::multipleCertificatesReady, this,
                &Controller::onCertificatesLoaded);

        commandHandlerRunThread->start();

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::onCertificatesLoaded()
{
    auto* cardEventMonitorThread = new CardEventMonitorThread(this, commandType());
    connect(cardEventMonitorThread, &ControllerChildThread::failure, this,
            &Controller::onCriticalFailure);
    connect(this, &Controller::stopCardEventMonitorThread, cardEventMonitorThread,
            &WaitForCardThread::requestInterruption);
    connect(cardEventMonitorThread, &CardEventMonitorThread::cardEvent, this, &Controller::onRetry);
    cardEventMonitorThread->start();
}

void Controller::disposeUI()
{
    delete window;
    window = nullptr;
}

void Controller::onConfirmCommandHandler(const EidCertificateAndPinInfo& certAndPinInfo)
{
    emit stopCardEventMonitorThread();

    try {
        auto* commandHandlerConfirmThread =
            new CommandHandlerConfirmThread(this, *commandHandler, window, certAndPinInfo);
        connect(commandHandlerConfirmThread, &CommandHandlerConfirmThread::completed, this,
                &Controller::onCommandHandlerConfirmCompleted);
        connect(commandHandlerConfirmThread, &ControllerChildThread::failure, this,
                &Controller::onCriticalFailure);
        connectRetry(commandHandlerConfirmThread);

        commandHandlerConfirmThread->start();

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::onCommandHandlerConfirmCompleted(const QVariantMap& res)
{
    REQUIRE_NON_NULL(window)

    qDebug() << "Command completed";

    // Schedule application exit when the UI dialog is destroyed.
    connect(window, &WebEidUI::destroyed, this, &Controller::exit);

    try {
        _result = res;
        writeResponseToStdOut(isInStdinMode, res, commandHandler->commandType());
    } catch (const std::exception& error) {
        qCritical() << "Command" << std::string(commandType())
                    << "fatal error while writing response to stdout:" << error;
    }

    window->quit();
}

void Controller::onRetry()
{
    try {
        // Dispose the UI, it will be re-created during next execution.
        disposeUI();
        // Command handler signals are still connected, disconnect them so that they can be
        // reconnected during next execution.
        commandHandler->disconnect();
        // Before restarting, wait until child threads finish.
        waitForChildThreads();

        startCommandExecution();

    } catch (const std::exception& error) {
        onCriticalFailure(error.what());
    }
}

void Controller::connectRetry(const ControllerChildThread* childThread)
{
    REQUIRE_NON_NULL(childThread)
    REQUIRE_NON_NULL(window)

    disconnect(window, &WebEidUI::retry, nullptr, nullptr);

    connect(childThread, &ControllerChildThread::retry, window, &WebEidUI::onRetry);
    // This connection handles cancel events from PIN pad.
    connect(childThread, &ControllerChildThread::cancel, this, &Controller::onPinPadCancel);
    connect(window, &WebEidUI::retry, this, &Controller::onRetry);
}

void Controller::onDialogOK(const EidCertificateAndPinInfo& certAndPinInfo)
{
    if (commandHandler) {
        onConfirmCommandHandler(certAndPinInfo);
    } else {
        // This should not happen, and when it does, OK should be equivalent to cancel.
        onPinPadCancel();
    }
}

void Controller::onDialogCancel()
{
    REQUIRE_NON_NULL(window)

    qDebug() << "User cancelled";

    // Schedule application exit when the UI dialog is destroyed.
    connect(window, &WebEidUI::destroyed, this, &Controller::exit);

    _result = makeErrorObject(RESP_USER_CANCEL, QStringLiteral("User cancelled"));
    writeResponseToStdOut(isInStdinMode, _result, commandType());
}

void Controller::onPinPadCancel()
{
    REQUIRE_NON_NULL(window)

    onDialogCancel();
    window->quit();
}

void Controller::onCriticalFailure(const QString& error)
{
    emit stopCardEventMonitorThread();
    qCritical() << "Exiting due to command" << std::string(commandType())
                << "fatal error:" << error;
    _result =
        makeErrorObject(RESP_TECH_ERROR, QStringLiteral("Technical error, see application logs"));
    writeResponseToStdOut(isInStdinMode, _result, commandType());
    disposeUI();
    WebEidUI::showFatalError();
    exit();
}

void Controller::exit()
{
    if (window) {
        window->disconnect();
        window = nullptr;
    }
    waitForChildThreads();
    emit quit();
}

void Controller::waitForChildThreads()
{
    for (auto* thread : findChildren<QThread*>()) {
        qDebug() << "Interrupting thread" << uintptr_t(thread);
        thread->disconnect();
        thread->requestInterruption();
        ControllerChildThread::waitForControllerNotify.wakeAll();
        // Call processEvents() so that the UI doesn't freeze.
        while (thread->isRunning()) {
            thread->wait(100ms);
            QCoreApplication::processEvents();
        }
    }
}

CommandType Controller::commandType()
{
    return commandHandler ? commandHandler->commandType() : CommandType(CommandType::INSERT_CARD);
}
