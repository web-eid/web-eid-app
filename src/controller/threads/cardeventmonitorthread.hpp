/*
 * Copyright (c) 2021-2024 Estonian Information System Authority
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

class CardEventMonitorThread : public ControllerChildThread
{
    Q_OBJECT

public:
    using eid_ptr = electronic_id::ElectronicID::ptr;
    using eid_ptr_vector = std::vector<electronic_id::ElectronicID::ptr>;

    CardEventMonitorThread(QObject* parent, std::string commandType) :
        ControllerChildThread(std::move(commandType), parent)
    {
    }

    void run() override
    {
        QMutexLocker lock {&controllerChildThreadMutex};

        qDebug() << "Starting" << metaObject()->className() << uintptr_t(this) << "for command"
                 << commandType();

        auto initialCards = getSupportedCardsIgnoringExceptions();
        sortByReaderNameAndAtr(initialCards);

        while (!isInterruptionRequested()) {
            using namespace std::chrono_literals;
            waitForControllerNotify.wait(&controllerChildThreadMutex, 1s);

            eid_ptr_vector updatedCards {};

            try {
                updatedCards = electronic_id::availableSupportedCards();
                sortByReaderNameAndAtr(updatedCards);
            } catch (const std::exception& error) {
                // Ignore smart card layer errors, they will be handled during next card operation.
                qWarning() << metaObject()->className() << "ignoring" << commandType()
                           << "error:" << error;
            }

            // If interruption was requested during wait, exit without emitting.
            if (isInterruptionRequested()) {
                return;
            }

            // If there was a change in connected supported cards, exit after emitting a card event.
            if (!areEqualByReaderNameAndAtr(initialCards, updatedCards)) {
                qDebug() << metaObject()->className() << "card change detected";
                emit cardEvent();
                return;
            }
        }
    }

signals:
    void cardEvent();

private:
    void doRun() override
    {
        // Unused as run() has been overriden.
    }

    eid_ptr_vector getSupportedCardsIgnoringExceptions()
    {
        while (!isInterruptionRequested()) {
            try {
                return electronic_id::availableSupportedCards();
            } catch (const std::exception& error) {
                // Ignore smart card layer errors, they will be handled during next card operation.
                qWarning() << metaObject()->className() << "ignoring" << commandType()
                           << "error:" << error;
            }
            using namespace std::chrono_literals;
            waitForControllerNotify.wait(&controllerChildThreadMutex, 1s);
        }
        // Interruption was requested, return empty list.
        return {};
    }

    static void sortByReaderNameAndAtr(eid_ptr_vector& a)
    {
        std::sort(a.begin(), a.end(), [](const eid_ptr& c1, const eid_ptr& c2) {
            if (c1->smartcard().readerName() != c2->smartcard().readerName()) {
                return c1->smartcard().readerName() < c2->smartcard().readerName();
            }
            return c1->smartcard().atr() < c2->smartcard().atr();
        });
    }

    static bool areEqualByReaderNameAndAtr(const eid_ptr_vector& a, const eid_ptr_vector& b)
    {
        return std::equal(a.cbegin(), a.cend(), b.cbegin(), b.cend(),
                          [](const eid_ptr& c1, const eid_ptr& c2) {
                              return c1->smartcard().readerName() == c2->smartcard().readerName()
                                  && c1->smartcard().atr() == c2->smartcard().atr();
                          });
    }
};
