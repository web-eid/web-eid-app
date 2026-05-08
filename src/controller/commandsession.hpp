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

#include "commandhandler.hpp"

#include "utils/observer_ptr.hpp"

#include <QPointer>
#include <QThread>

#include <vector>

class ControllerChildThread;

class CommandSession : public QObject
{
    Q_OBJECT

public:
    enum class State : quint8 {
        Idle,
        WaitingForCard,
        ReadingCertificates,
        WaitingForUserConfirmation,
        Confirming,
        MonitoringCardChanges,
        Retrying,
        Completed,
        Cancelled,
        Failed,
    };
    Q_ENUM(State)

    CommandSession(CommandHandler::ptr handler, observer_ptr<WebEidUI> window);
    ~CommandSession() override;

    CommandType commandType() const { return commandHandler->commandType(); }
    State state() const noexcept { return currentState; }

    void start() noexcept;
    void stop() noexcept;

signals:
    void statusUpdate(RetriableError status);
    void retry(RetriableError error);
    void completed(const QVariantMap& result);
    void cancelled();
    void failed(const QString& error);
    void restartRequested();

private slots:
    void
    onCardsAvailable(const std::vector<electronic_id::ElectronicID::ptr>& availableEids) noexcept;
    void onCertificatesLoaded() noexcept;
    void confirm(const EidCertificateAndPinInfo& certAndPinInfo) noexcept;
    void cancel() noexcept;
    void fail(const QString& error) noexcept;
    void requestRestart() noexcept;
    void onCommandHandlerConfirmCompleted(const QVariantMap& result) noexcept;

private:
    void startCardWait();
    void connectChildThreadSignals(ControllerChildThread* childThread);
    void trackThread(QThread* thread);
    void stopChildThreads() noexcept;
    void setState(State state) noexcept { currentState = state; }

    CommandHandler::ptr commandHandler;
    observer_ptr<WebEidUI> window = nullptr;
    std::vector<QPointer<QThread>> childThreads;
    State currentState = State::Idle;

signals:
    void stopCardEventMonitorThread();
};
