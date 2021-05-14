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
#include "retriableerror.hpp"
#include "qeid.hpp"
#include "logging.hpp"

#include <QThread>
#include <QMutexLocker>
#include <QWaitCondition>

class ControllerChildThread : public QThread
{
    Q_OBJECT

public:
    void run() override
    {
        QMutexLocker lock {&controllerChildThreadMutex};

        beforeRun();

        try {
            doRun();
            qInfo() << className << uintptr_t(this) << "for command" << commandType()
                    << "completed successfully";

        } catch (const CommandHandlerVerifyPinFailed& error) {
            qWarning() << "Command" << commandType() << "PIN verification failed:" << error;
        }
        CATCH_PCSC_CPP_RETRIABLE_ERRORS(warnAndEmitRetry)
        CATCH_LIBELECTRONIC_ID_RETRIABLE_ERRORS(warnAndEmitRetry)
        catch (const electronic_id::VerifyPinFailed& error)
        {
            if (error.status() == electronic_id::VerifyPinFailed::Status::PIN_ENTRY_CANCEL) {
                qInfo() << "Command" << commandType() << "canceled";
                emit cancel();
            } else {
                qCritical() << "Command" << commandType() << "fatal error:" << error;
                emit failure(error.what());
            }
        }
        catch (const std::exception& error)
        {
            qCritical() << "Command" << commandType() << "fatal error:" << error;
            emit failure(error.what());
        }
    }

    static QWaitCondition waitForControllerNotify;

signals:
    void cancel();
    void retry(const RetriableError error);
    void failure(const QString& error);

protected:
    explicit ControllerChildThread(QObject* parent) : QThread(parent) {}
    ~ControllerChildThread() override
    {
        // Avoid throwing in destructor.
        try {
            qDebug() << className << uintptr_t(this) << "destroyed";
        } catch (...) {
        }
    }

    void beforeRun()
    {
        // Cannot use virtual calls in constructor, have to initialize the class name here.
        className = metaObject()->className();
        qDebug() << "Starting" << className << uintptr_t(this) << "for command" << commandType();
    }

    static const unsigned long ONE_SECOND = 1000;

    static QMutex controllerChildThreadMutex;
    QString className;

private:
    virtual void doRun() = 0;
    virtual const std::string& commandType() const = 0;

    void warnAndEmitRetry(const RetriableError errorCode, const std::exception& error)
    {
        WARN_RETRIABLE_ERROR(commandType(), errorCode, error);
        emit retry(errorCode);
    }
};
