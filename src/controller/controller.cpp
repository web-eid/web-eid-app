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

#include "application.hpp"
#include "nativemessagingsession.hpp"

#include "utils/utils.hpp"

namespace
{

const QString RESP_TECH_ERROR = QStringLiteral("ERR_WEBEID_NATIVE_FATAL");
const QString RESP_USER_CANCEL = QStringLiteral("ERR_WEBEID_USER_CANCELLED");

QVariantMap makeErrorObject(const QString& errorCode, const QString& errorMessage) noexcept
{
    return {{QStringLiteral("error"),
             QVariantMap {
                 {QStringLiteral("code"), errorCode},
                 {QStringLiteral("message"), errorMessage},
             }}};
}

} // namespace

void Controller::run() noexcept
try {
    initializeResponseSink();

    qInfo() << QCoreApplication::applicationName() << "app"
            << QCoreApplication::applicationVersion() << "running in"
            << (command ? "command-line" : "stdin/stdout") << "mode";

    if (!command) {
        auto* nativeMessagingSession = dynamic_cast<NativeMessagingSession*>(responseSink.get());
        REQUIRE_NON_NULL(nativeMessagingSession)

        nativeMessagingSession->writeHandshake();
        command = nativeMessagingSession->readCommand();
    }

    REQUIRE_NON_NULL(command)
    switch (command->first) {
    case CommandType::ABOUT:
        WebEidUI::showAboutPage();
        return;
    case CommandType::QUIT:
        qInfo() << "Quit requested, exiting";
        writeResult({}, CommandType::QUIT);
        emit quit();
        return;
    default:
        break;
    }

    startCommandExecution();

} catch (const std::exception& error) {
    onCriticalFailure(error.what());
}

void Controller::initializeResponseSink()
{
    if (responseSink) {
        return;
    }

    // QUIT is a native-messaging-only command, but tests can inject it directly.
    if (!command || command->first == CommandType::QUIT) {
        responseSink = std::make_unique<NativeMessagingSession>();
    } else {
        responseSink = std::make_unique<CommandLineResponseSink>();
    }
}

void Controller::startCommandExecution()
{
    REQUIRE_NON_NULL(command)

    auto commandHandler = getCommandHandler(*command);
    const auto type = commandHandler->commandType();

    createWindow(type);

    commandSession = std::make_unique<CommandSession>(std::move(commandHandler), window);
    connect(commandSession.get(), &CommandSession::statusUpdate, this, &Controller::statusUpdate);
    connect(commandSession.get(), &CommandSession::retry, this, &Controller::retry);
    connect(commandSession.get(), &CommandSession::completed, this,
            &Controller::onCommandSessionCompleted);
    connect(commandSession.get(), &CommandSession::cancelled, this,
            &Controller::onCommandSessionCancelled);
    connect(commandSession.get(), &CommandSession::failed, this, &Controller::onCriticalFailure);
    connect(commandSession.get(), &CommandSession::restartRequested, this, &Controller::onRetry);

    commandSession->start();
}

void Controller::createWindow(CommandType commandType)
{
    window = WebEidUI::createAndShowDialog(commandType);
    connect(this, &Controller::statusUpdate, window, &WebEidUI::onSmartCardStatusUpdate);
    connect(this, &Controller::retry, window, &WebEidUI::onRetry);
    connect(window, &WebEidUI::destroyed, this, [this] { window = nullptr; });
}

void Controller::onCommandSessionCompleted(const QVariantMap& res) noexcept
try {
    _result = res;
    writeResult(res, commandType());
    exit();
} catch (const std::exception& error) {
    onCriticalFailure(error.what());
}

void Controller::onCommandSessionCancelled() noexcept
try {
    _result = makeErrorObject(RESP_USER_CANCEL, QStringLiteral("User cancelled"));
    writeResult(_result, commandType());
    exit();
} catch (const std::exception& e) {
    onCriticalFailure(e.what());
}

void Controller::onRetry() noexcept
try {
    disposeCommandSession();
    disposeUI();
    startCommandExecution();

} catch (const std::exception& error) {
    onCriticalFailure(error.what());
}

void Controller::onCriticalFailure(const QString& error) noexcept
try {
    qCritical() << "Exiting due to command" << commandType() << "fatal error:" << error;
    _result =
        makeErrorObject(RESP_TECH_ERROR, QStringLiteral("Technical error, see application logs"));
    disposeCommandSession();
    disposeUI();
    if (responseSink && qApp->isSafariExtensionContainingApp()) {
        writeResult(_result, commandType());
    }
    WebEidUI::showFatalError();
    if (responseSink && !qApp->isSafariExtensionContainingApp()) {
        // Write the error response after showing the fatal error dialog. Chrome closes the
        // application immediately after this, so the dialog may not otherwise be visible.
        writeResult(_result, commandType());
    }
    exit();
} catch (const std::exception& e) {
    qCritical() << "Failed to write stdout" << e.what();
    exit();
}

void Controller::disposeCommandSession() noexcept
{
    if (commandSession) {
        commandSession->disconnect();
        commandSession->stop();
        auto* session = commandSession.release();
        session->deleteLater();
    }
}

void Controller::disposeUI() noexcept
{
    if (window) {
        window->disconnect();
        window->forceClose();
        window->deleteLater();
        window = nullptr;
    }
}

void Controller::exit() noexcept
{
    disposeCommandSession();
    disposeUI();
    emit quit();
}

CommandType Controller::commandType() const noexcept
{
    if (commandSession) {
        return commandSession->commandType();
    }
    return command ? command->first : CommandType(CommandType::INSERT_CARD);
}

void Controller::writeResult(const QVariantMap& result, CommandType resultCommandType)
{
    REQUIRE_NON_NULL(responseSink)
    responseSink->writeResult(result, resultCommandType);
}
