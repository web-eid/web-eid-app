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
#include "qeid.hpp"
#include "logging.hpp"

#include <QThread>
#include <QMutexLocker>

class ControllerChildThread : public QThread
{
    Q_OBJECT

public:
    void run() override
    {
        // Cannot use virtual calls in constructor, have to initialize the class name here.
        className = metaObject()->className();

        static QMutex mutex;
        QMutexLocker lock {&mutex};

        try {
            qDebug() << "Starting" << className << "for command" << commandType();
            doRun();
            qInfo() << className << "for command" << commandType() << "completed successfully";

        } catch (const CommandHandlerVerifyPinFailed& error) {
            qWarning() << "Command" << commandType() << "PIN verification failed:" << error;

        } catch (const CommandHandlerRetriableError& error) {
            qWarning() << "Command" << commandType() << "retriable error:" << error;
            emit retry(error.what());

        } catch (const std::exception& error) {
            qCritical() << "Command" << commandType() << "fatal error:" << error;
            emit failure(error.what());
        }
    }

signals:
    void failure(const QString& error);
    void retry(const QString& error);

protected:
    explicit ControllerChildThread(QObject* parent) : QThread(parent) {}
    ~ControllerChildThread() override
    {
        // Avoid throwing in destructor.
        try {
            qDebug() << className << "destroyed";
        } catch (...) {
        }
    }

private:
    virtual void doRun() = 0;
    virtual const std::string& commandType() const = 0;

    QString className;
};
