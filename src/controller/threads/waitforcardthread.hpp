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

#include "controllerchildthread.hpp"

class WaitForCardThread : public ControllerChildThread
{
    Q_OBJECT

public:
    explicit WaitForCardThread(QObject* parent) : ControllerChildThread(parent) {}

signals:
    void cardsAvailable(const std::vector<electronic_id::CardInfo::ptr>& cardInfo);
    void statusUpdate(const RetriableError status);

private:
    void doRun() override
    {
        while (!attemptCardSelection() && !isInterruptionRequested()) {
            waitForControllerNotify.wait(&controllerChildThreadMutex, ONE_SECOND);
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
            emit statusUpdate(toRetriableError(failure.reason()));
            return false;
        }
        CATCH_PCSC_CPP_RETRIABLE_ERRORS(return warnAndEmitStatusUpdate)
        CATCH_LIBELECTRONIC_ID_RETRIABLE_ERRORS(return warnAndEmitStatusUpdate)
        catch (const std::exception& error) { emit failure(error.what()); }
        return true;
    }

    bool warnAndEmitStatusUpdate(const RetriableError errorCode, const std::exception& error)
    {
        WARN_RETRIABLE_ERROR(commandType(), errorCode, error);
        emit statusUpdate(errorCode);
        return false;
    }

    const std::string& commandType() const override
    {
        static const std::string cmdType = CommandType(CommandType::INSERT_CARD);
        return cmdType;
    }
};
