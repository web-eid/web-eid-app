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

#include "electronic-id/electronic-id.hpp"
#include "pcsc-cpp/pcsc-cpp-utils.hpp"

#include <QMetaType>

class RetriableError
{
    Q_GADGET
public:
    enum Error : quint8 {
        // default
        UNKNOWN_ERROR,
        // libpcsc-cpp
        SMART_CARD_SERVICE_IS_NOT_RUNNING,
        NO_SMART_CARD_READERS_FOUND,
        NO_SMART_CARDS_FOUND,
        FAILED_TO_COMMUNICATE_WITH_CARD_OR_READER,
        SMART_CARD_WAS_REMOVED,
        SMART_CARD_TRANSACTION_FAILED,
        SCARD_ERROR,
        // libelectronic-id
        SMART_CARD_CHANGE_REQUIRED,
        SMART_CARD_COMMAND_ERROR,
        PKCS11_TOKEN_NOT_PRESENT,
        PKCS11_TOKEN_REMOVED,
        PKCS11_ERROR,
        // AutoSelectFailed::Reason
        UNSUPPORTED_CARD,
        // CertificateReader::run
        NO_VALID_CERTIFICATE_AVAILABLE,
        PIN_VERIFY_DISABLED,
    };
    Q_ENUM(Error)

    constexpr RetriableError(Error error = UNKNOWN_ERROR) : value(error) {}
    constexpr explicit RetriableError(const electronic_id::AutoSelectFailed::Reason reason)
    {
        switch (reason) {
            using enum electronic_id::AutoSelectFailed::Reason;
        case SERVICE_NOT_RUNNING:
            value = SMART_CARD_SERVICE_IS_NOT_RUNNING;
            break;
        case NO_READERS:
            value = NO_SMART_CARD_READERS_FOUND;
            break;
        case SINGLE_READER_NO_CARD:
        case MULTIPLE_READERS_NO_CARD:
            value = NO_SMART_CARDS_FOUND;
            break;
        case SINGLE_READER_UNSUPPORTED_CARD:
        case MULTIPLE_READERS_NO_SUPPORTED_CARD:
            value = UNSUPPORTED_CARD;
            break;
        default:
            value = UNKNOWN_ERROR;
        }
    }

    constexpr operator Error() const { return value; }

    static RetriableError catchRetriableError()
    {
        try {
            throw;
        } catch (const pcsc_cpp::ScardServiceNotRunningError& /*error*/) {
            return SMART_CARD_SERVICE_IS_NOT_RUNNING;
        } catch (const pcsc_cpp::ScardNoReadersError& /*error*/) {
            return NO_SMART_CARD_READERS_FOUND;
        } catch (const pcsc_cpp::ScardNoCardError& /*error*/) {
            return NO_SMART_CARDS_FOUND;
        } catch (const pcsc_cpp::ScardCardCommunicationFailedError& /*error*/) {
            return FAILED_TO_COMMUNICATE_WITH_CARD_OR_READER;
        } catch (const pcsc_cpp::ScardCardRemovedError& /*error*/) {
            return SMART_CARD_WAS_REMOVED;
        } catch (const pcsc_cpp::ScardTransactionFailedError& /*error*/) {
            return SMART_CARD_TRANSACTION_FAILED;
        } catch (const pcsc_cpp::ScardError& /*error*/) {
            return SCARD_ERROR;
        } catch (const electronic_id::SmartCardChangeRequiredError& /*error*/) {
            return SMART_CARD_CHANGE_REQUIRED;
        } catch (const electronic_id::SmartCardError& /*error*/) {
            return SMART_CARD_COMMAND_ERROR;
        } catch (const electronic_id::Pkcs11TokenNotPresent& /*error*/) {
            return PKCS11_TOKEN_NOT_PRESENT;
        } catch (const electronic_id::Pkcs11TokenRemoved& /*error*/) {
            return PKCS11_TOKEN_REMOVED;
        } catch (const electronic_id::Pkcs11Error& /*error*/) {
            return PKCS11_ERROR;
        } catch (...) {
            return UNKNOWN_ERROR;
        }
    }

private:
    Error value;
};

#define WARN_RETRIABLE_ERROR(commandType, errorCode, error)                                        \
    qWarning().nospace() << "Command " << commandType << " retriable error " << errorCode << ": "  \
                         << error

Q_DECLARE_METATYPE(RetriableError)
