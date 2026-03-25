// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "electronic-id/electronic-id.hpp"

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

    constexpr RetriableError(Error error = UNKNOWN_ERROR) : value(error) { }
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

    explicit RetriableError(std::exception_ptr eptr)
    {
        value = UNKNOWN_ERROR;
        try {
            if (eptr)
                std::rethrow_exception(eptr);
        } catch (const pcsc_cpp::ScardServiceNotRunningError& /*error*/) {
            value = SMART_CARD_SERVICE_IS_NOT_RUNNING;
        } catch (const pcsc_cpp::ScardNoReadersError& /*error*/) {
            value = NO_SMART_CARD_READERS_FOUND;
        } catch (const pcsc_cpp::ScardNoCardError& /*error*/) {
            value = NO_SMART_CARDS_FOUND;
        } catch (const pcsc_cpp::ScardCardCommunicationFailedError& /*error*/) {
            value = FAILED_TO_COMMUNICATE_WITH_CARD_OR_READER;
        } catch (const pcsc_cpp::ScardCardRemovedError& /*error*/) {
            value = SMART_CARD_WAS_REMOVED;
        } catch (const pcsc_cpp::ScardTransactionFailedError& /*error*/) {
            value = SMART_CARD_TRANSACTION_FAILED;
        } catch (const pcsc_cpp::ScardError& /*error*/) {
            value = SCARD_ERROR;
        } catch (const electronic_id::SmartCardChangeRequiredError& /*error*/) {
            value = SMART_CARD_CHANGE_REQUIRED;
        } catch (const electronic_id::SmartCardError& /*error*/) {
            value = SMART_CARD_COMMAND_ERROR;
        } catch (const electronic_id::Pkcs11TokenNotPresent& /*error*/) {
            value = PKCS11_TOKEN_NOT_PRESENT;
        } catch (const electronic_id::Pkcs11TokenRemoved& /*error*/) {
            value = PKCS11_TOKEN_REMOVED;
        } catch (const electronic_id::Pkcs11Error& /*error*/) {
            value = PKCS11_ERROR;
        } catch (...) {
        }
    }

    constexpr operator Error() const { return value; }

private:
    Error value;
};

#define WARN_RETRIABLE_ERROR(commandType, errorCode, error)                                        \
    qWarning().nospace() << "Command " << commandType << " retriable error " << errorCode << ": "  \
                         << error

Q_DECLARE_METATYPE(RetriableError)
