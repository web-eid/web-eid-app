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

#include "controllerchildthread.hpp"

class WaitForCardThread : public ControllerChildThread
{
    Q_OBJECT

public:
    explicit WaitForCardThread(QObject* parent) :
        ControllerChildThread(CommandType(CommandType::INSERT_CARD), parent)
    {
    }

signals:
    void cardsAvailable(const std::vector<electronic_id::ElectronicID::ptr>& eids);
    void statusUpdate(RetriableError status);

private:
    void doRun() override
    {
        while (!attemptCardSelection() && !isInterruptionRequested()) {
            using namespace std::chrono_literals;
            waitForControllerNotify.wait(&controllerChildThreadMutex, 1s);
        }
    }

    bool attemptCardSelection()
    {
        try {
            const auto availableCardInfos = electronic_id::availableSupportedCards();

            if (!availableCardInfos.empty()) {
                emit cardsAvailable(availableCardInfos);
            } else {
                // This should never happen.
                emit failure(QString(__func__) + ": empty available supported card list");
            }
        } catch (const electronic_id::AutoSelectFailed& failure) {
            emit statusUpdate(RetriableError(failure.reason()));
            return false;
        } catch (const std::exception& error) {
            if (auto errorCode = RetriableError::catchRetriableError();
                errorCode != RetriableError::UNKNOWN_ERROR) {
                WARN_RETRIABLE_ERROR(commandType(), errorCode, error);
                emit statusUpdate(errorCode);
                return false;
            }
            emit failure(error.what());
        }
        return true;
    }
};
