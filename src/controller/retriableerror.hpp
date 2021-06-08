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

#include "electronic-id/electronic-id.hpp"
#include "pcsc-cpp/pcsc-cpp-utils.hpp"

#include <QMetaType>
#include <QDebug>

enum class RetriableError {
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
    // default
    UNKNOWN_ERROR
};

Q_DECLARE_METATYPE(RetriableError)

QDebug& operator<<(QDebug& d, const RetriableError);

RetriableError toRetriableError(const electronic_id::AutoSelectFailed::Reason reason);

// Define retriable error handling in one place so that it can be reused.

#define CATCH_PCSC_CPP_RETRIABLE_ERRORS(ERROR_HANDLER)                                             \
    catch (const pcsc_cpp::ScardServiceNotRunningError& error)                                     \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::SMART_CARD_SERVICE_IS_NOT_RUNNING, error);                   \
    }                                                                                              \
    catch (const pcsc_cpp::ScardNoReadersError& error)                                             \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::NO_SMART_CARD_READERS_FOUND, error);                         \
    }                                                                                              \
    catch (const pcsc_cpp::ScardNoCardError& error)                                                \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::NO_SMART_CARDS_FOUND, error);                                \
    }                                                                                              \
    catch (const pcsc_cpp::ScardCardCommunicationFailedError& error)                               \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::FAILED_TO_COMMUNICATE_WITH_CARD_OR_READER, error);           \
    }                                                                                              \
    catch (const pcsc_cpp::ScardCardRemovedError& error)                                           \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::SMART_CARD_WAS_REMOVED, error);                              \
    }                                                                                              \
    catch (const pcsc_cpp::ScardTransactionFailedError& error)                                     \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::SMART_CARD_TRANSACTION_FAILED, error);                       \
    }                                                                                              \
    catch (const pcsc_cpp::ScardError& error) { ERROR_HANDLER(RetriableError::SCARD_ERROR, error); }

#define CATCH_LIBELECTRONIC_ID_RETRIABLE_ERRORS(ERROR_HANDLER)                                     \
    catch (const electronic_id::SmartCardChangeRequiredError& error)                               \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::SMART_CARD_CHANGE_REQUIRED, error);                          \
    }                                                                                              \
    catch (const electronic_id::SmartCardError& error)                                             \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::SMART_CARD_COMMAND_ERROR, error);                            \
    }                                                                                              \
    catch (const electronic_id::Pkcs11TokenNotPresent& error)                                      \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::PKCS11_TOKEN_NOT_PRESENT, error);                            \
    }                                                                                              \
    catch (const electronic_id::Pkcs11TokenRemoved& error)                                         \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::PKCS11_TOKEN_REMOVED, error);                                \
    }                                                                                              \
    catch (const electronic_id::Pkcs11Error& error)                                                \
    {                                                                                              \
        ERROR_HANDLER(RetriableError::PKCS11_ERROR, error);                                        \
    }

#define WARN_RETRIABLE_ERROR(commandType, errorCode, error)                                        \
    qWarning().nospace() << "Command " << commandType << " retriable error " << errorCode << ": "  \
                         << error
