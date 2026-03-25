// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

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
            if (RetriableError errorCode(std::current_exception());
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
