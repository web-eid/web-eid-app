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

#include "qeid.hpp"

#include "pcsc-cpp/pcsc-cpp.hpp"

#include <QDebug>
#include <QThread>

class ReaderMonitorThread : public QThread
{
    Q_OBJECT

public:
    explicit ReaderMonitorThread(QObject* parent = nullptr) : QThread(parent) {}
    ~ReaderMonitorThread()
    {
        requestInterruption();
        wait();
    }

    void run() override
    {
        try {
            auto isFinished = attemptCardSelection();
            while (!isFinished && !isInterruptionRequested()) {
                msleep(500); // sleep for half a second
                isFinished = attemptCardSelection();
            }
        } catch (const std::exception& e) {
            emit readerMonitorFailure(e.what());
        }
        qDebug() << "Reader monitor thread finished, deleting thread object";
        deleteLater();
    }

signals:
    void cardReady(electronic_id::CardInfo::ptr cardInfo);
    void statusUpdate(electronic_id::AutoSelectFailed::Reason);
    void readerMonitorFailure(const QString& error);

private:
    bool attemptCardSelection()
    {
        try {
            const auto selectedCardInfo = electronic_id::autoSelectSupportedCard();

            if (selectedCardInfo) {
                emit cardReady(selectedCardInfo);
            } else {
                // This should never happen.
                emit readerMonitorFailure("Internal error: null selected card info");
            }
        } catch (const electronic_id::AutoSelectFailed& failure) {
            emit statusUpdate(failure.reason());
            return false;
        } catch (const std::exception& error) {
            emit readerMonitorFailure(error.what());
        }
        return true;
    }
};
