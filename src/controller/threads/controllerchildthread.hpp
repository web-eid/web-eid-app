// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "commandhandler.hpp"
#include "logging.hpp"
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
            if (RetriableError errorCode(std::current_exception());
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
    explicit ControllerChildThread(CommandType _cmdType, QObject* parent) noexcept :
        QThread(parent), cmdType(_cmdType)
    {
        // When the thread is finished call deleteLater() on it to free the thread object. Although
        // the thread objects are freed through the Qt object tree ownership system anyway, it is
        // better to delete them immediately when they finish.
        connect(this, &ControllerChildThread::finished, this, [this] {
            qDebug() << metaObject()->className() << uintptr_t(this) << "finished";
            deleteLater();
        });
    }

    CommandType commandType() const { return cmdType; }

    static QMutex controllerChildThreadMutex;

private:
    virtual void doRun() = 0;
    const CommandType cmdType;
};
