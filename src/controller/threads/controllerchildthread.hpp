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
#include "logging.hpp"
#include "qeid.hpp"
#include "retriableerror.hpp"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QThread>
#include <QWaitCondition>

class ControllerChildThread : public QThread
{
    Q_OBJECT

public:
    void run() override
    {
        QMutexLocker lock {&controllerChildThreadMutex};

        qDebug() << "Starting" << metaObject()->className() << uintptr_t(this) << "for command"
                 << commandType();

        try {
            doRun();
            qInfo() << metaObject()->className() << uintptr_t(this) << "for command"
                    << commandType() << "completed successfully";

        } catch (const CommandHandlerVerifyPinFailed& error) {
            qWarning() << "Command" << commandType() << "PIN verification failed:" << error;
        } catch (const electronic_id::VerifyPinFailed& error) {
            switch (error.status()) {
                using enum electronic_id::VerifyPinFailed::Status;
            case PIN_ENTRY_CANCEL:
                qInfo() << "Command" << commandType() << "canceled";
                emit cancel();
                break;
            case INVALID_PIN_LENGTH:
                qInfo() << "Command" << commandType() << "invalid PIN length";
                break;
            case PIN_ENTRY_TIMEOUT:
                qInfo() << "Command" << commandType() << "PIN entry timeout";
                emit cancel();
                break;
            case PIN_BLOCKED:
                qInfo() << "Command" << commandType() << "PIN blocked";
                break;
            default:
                qCritical() << "Command" << commandType() << "fatal error:" << error;
                emit failure(error.what());
            }
        } catch (const std::exception& error) {
            if (auto errorCode = RetriableError::catchRetriableError();
                errorCode != RetriableError::UNKNOWN_ERROR) {
                WARN_RETRIABLE_ERROR(commandType(), errorCode, error);
                emit retry(errorCode);
            } else {
                qCritical() << "Command" << commandType() << "fatal error:" << error;
                emit failure(error.what());
            }
        }
    }

    static QWaitCondition waitForControllerNotify;

signals:
    void cancel();
    void retry(RetriableError error);
    void failure(const QString& error);

protected:
    explicit ControllerChildThread(std::string _cmdType, QObject* parent) noexcept :
        QThread(parent), cmdType(std::move(_cmdType))
    {
        // When the thread is finished call deleteLater() on it to free the thread object. Although
        // the thread objects are freed through the Qt object tree ownership system anyway, it is
        // better to delete them immediately when they finish.
        connect(this, &ControllerChildThread::finished, this, [this] {
            qDebug() << metaObject()->className() << uintptr_t(this) << "finished";
            deleteLater();
        });
    }

    const std::string& commandType() const { return cmdType; }

    static QMutex controllerChildThreadMutex;

private:
    virtual void doRun() = 0;
    const std::string cmdType;
};
