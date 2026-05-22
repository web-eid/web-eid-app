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

#include "commandsession.hpp"

#include "threads/cardeventmonitorthread.hpp"
#include "threads/commandhandlerconfirmthread.hpp"
#include "threads/commandhandlerrunthread.hpp"
#include "threads/waitforcardthread.hpp"

#include "utils/utils.hpp"

using namespace pcsc_cpp;
using namespace electronic_id;

CommandSession::CommandSession(CommandHandler::ptr handler, observer_ptr<WebEidUI> w) :
    commandHandler(std::move(handler)), window(w)
{
    REQUIRE_NON_NULL(commandHandler)
    REQUIRE_NON_NULL(window)

    connect(window, &WebEidUI::retry, this, &CommandSession::requestRestart);
    connect(window, &WebEidUI::accepted, this, &CommandSession::confirm);
    connect(window, &WebEidUI::rejected, this, &CommandSession::cancel);
    connect(window, &WebEidUI::failure, this, &CommandSession::fail);
    connect(window, &WebEidUI::waitingForPinPad, this, &CommandSession::confirm);
}

CommandSession::~CommandSession()
{
    stop();
}

void CommandSession::start() noexcept
try {
    startCardWait();
} catch (const std::exception& error) {
    fail(error.what());
}

void CommandSession::stop() noexcept
{
    stopChildThreads();
    if (commandHandler) {
        commandHandler->disconnect();
    }
}

void CommandSession::startCardWait()
{
    REQUIRE_NON_NULL(commandHandler)

    setState(State::WaitingForCard);

    auto* waitForCardThread = new WaitForCardThread(this);
    trackThread(waitForCardThread);
    connect(this, &CommandSession::stopCardEventMonitorThread, waitForCardThread,
            &WaitForCardThread::requestInterruption);
    connect(waitForCardThread, &WaitForCardThread::statusUpdate, this,
            &CommandSession::statusUpdate);
    connect(waitForCardThread, &WaitForCardThread::cardsAvailable, this,
            &CommandSession::onCardsAvailable);
    connectChildThreadSignals(waitForCardThread);

    // When the command handler retrieves certificates successfully, start card event monitoring
    // while the user enters the PIN.
    connect(commandHandler.get(), &CommandHandler::singleCertificateReady, this,
            &CommandSession::onCertificatesLoaded);
    connect(commandHandler.get(), &CommandHandler::multipleCertificatesReady, this,
            &CommandSession::onCertificatesLoaded);
    connect(commandHandler.get(), &CommandHandler::verifyPinFailed, this,
            &CommandSession::onCertificatesLoaded);

    waitForCardThread->start();
}

void CommandSession::onCardsAvailable(
    const std::vector<electronic_id::ElectronicID::ptr>& availableEids) noexcept
try {
    REQUIRE_NON_NULL(commandHandler)
    REQUIRE_NON_NULL(window)
    REQUIRE_NOT_EMPTY_CONTAINS_NON_NULL_PTRS(availableEids)

    setState(State::ReadingCertificates);

    for (const auto& card : availableEids) {
        const auto protocol =
            card->smartcard().protocol() == SmartCard::Protocol::T0 ? "T=0" : "T=1";
        qInfo() << "Card" << card->name() << "in reader" << card->smartcard().readerName()
                << "using protocol" << protocol;
    }

    window->showWaitingForCardPage(commandHandler->commandType());

    commandHandler->connectSignals(window);

    auto* commandHandlerRunThread =
        new CommandHandlerRunThread(this, *commandHandler, availableEids);
    trackThread(commandHandlerRunThread);
    connectChildThreadSignals(commandHandlerRunThread);

    commandHandlerRunThread->start();

} catch (const std::exception& error) {
    fail(error.what());
}

void CommandSession::onCertificatesLoaded() noexcept
try {
    setState(State::MonitoringCardChanges);

    auto* cardEventMonitorThread = new CardEventMonitorThread(this, commandType());
    trackThread(cardEventMonitorThread);
    connect(this, &CommandSession::stopCardEventMonitorThread, cardEventMonitorThread,
            &CardEventMonitorThread::requestInterruption);
    connect(cardEventMonitorThread, &CardEventMonitorThread::cardEvent, this,
            &CommandSession::requestRestart);
    connectChildThreadSignals(cardEventMonitorThread);
    cardEventMonitorThread->start();
} catch (const std::exception& error) {
    fail(error.what());
}

void CommandSession::confirm(const EidCertificateAndPinInfo& certAndPinInfo) noexcept
try {
    REQUIRE_NON_NULL(commandHandler)

    setState(State::Confirming);
    emit stopCardEventMonitorThread();

    auto* commandHandlerConfirmThread =
        new CommandHandlerConfirmThread(this, *commandHandler, window, certAndPinInfo);
    trackThread(commandHandlerConfirmThread);
    connect(commandHandlerConfirmThread, &CommandHandlerConfirmThread::completed, this,
            &CommandSession::onCommandHandlerConfirmCompleted);
    connectChildThreadSignals(commandHandlerConfirmThread);

    commandHandlerConfirmThread->start();

} catch (const std::exception& error) {
    fail(error.what());
}

void CommandSession::onCommandHandlerConfirmCompleted(const QVariantMap& result) noexcept
{
    qDebug() << "Command completed";
    setState(State::Completed);
    stopChildThreads();
    emit completed(result);
}

void CommandSession::cancel() noexcept
{
    qDebug() << "User cancelled";
    setState(State::Cancelled);
    stopChildThreads();
    emit cancelled();
}

void CommandSession::fail(const QString& error) noexcept
{
    if (currentState == State::Failed) {
        return;
    }

    setState(State::Failed);
    stopChildThreads();
    emit failed(error);
}

void CommandSession::requestRestart() noexcept
{
    setState(State::Retrying);
    stopChildThreads();
    emit restartRequested();
}

void CommandSession::connectChildThreadSignals(ControllerChildThread* childThread)
{
    REQUIRE_NON_NULL(childThread)
    connect(childThread, &ControllerChildThread::failure, this, &CommandSession::fail);
    connect(childThread, &ControllerChildThread::retry, this, &CommandSession::retry);
    connect(childThread, &ControllerChildThread::cancel, this, &CommandSession::cancel);
}

void CommandSession::trackThread(QThread* thread)
{
    REQUIRE_NON_NULL(thread)
    childThreads.push_back(thread);
}

void CommandSession::stopChildThreads() noexcept
{
    emit stopCardEventMonitorThread();

    for (const auto& thread : childThreads) {
        if (!thread) {
            continue;
        }
        qDebug() << "Interrupting thread" << uintptr_t(thread.data());
        disconnect(this, nullptr, thread, nullptr);
        thread->disconnect(this);
        thread->requestInterruption();
    }

    ControllerChildThread::waitForControllerNotify.wakeAll();

    for (const auto& thread : childThreads) {
        if (thread) {
            thread->wait();
        }
    }

    childThreads.clear();
}
