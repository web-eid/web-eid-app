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

#include "sign.hpp"

#include "signauthutils.hpp"
#include "utils/utils.hpp"

#include <QScopeGuard>

using namespace electronic_id;

namespace
{

QPair<QString, QVariantMap> signHash(const ElectronicID& eid, pcsc_cpp::byte_vector&& pin,
                                     const QByteArray& docHash, const HashAlgorithm hashAlgo)
{
    const auto hashBytes = pcsc_cpp::byte_vector {docHash.begin(), docHash.end()};
    const auto signature = eid.signWithSigningKey(std::move(pin), hashBytes, hashAlgo);

    const auto signatureBase64 =
        QByteArray::fromRawData(reinterpret_cast<const char*>(signature.first.data()),
                                int(signature.first.size()))
            .toBase64();

    return {signatureBase64, signatureAlgoToVariantMap(signature.second)};
}

} // namespace

Sign::Sign(const CommandWithArguments& cmd) : CertificateReader(cmd)
{
    const auto& arguments = cmd.second;

    requireArgumentsAndOptionalLang(
        {"hash", "hashFunction", "certificate", "origin"}, arguments,
        "\"hash\": \"<Base64-encoded document hash>\", "
        "\"hashFunction\": \"<the hash algorithm that was used for computing 'hash', any of "
            + HashAlgorithm::allSupportedAlgorithmNames()
            + ">\", \"certificate\": \"<Base64-encoded user eID certificate previously "
              "retrieved with get-cert>\", "
              "\"origin\": \"<origin URL>\"");

    validateAndStoreDocHashAndHashAlgo(arguments);

    userEidCertificateFromArgs = QByteArray::fromBase64(
        validateAndGetArgument<QString>(QStringLiteral("certificate"), arguments, false)
            .toLatin1());
    validateAndStoreOrigin(arguments);
}

void Sign::emitCertificatesReady(const std::vector<CardCertificateAndPinInfo>& cardCertAndPinInfos)
{
    const CardCertificateAndPinInfo* cardWithCertificateFromArgs = nullptr;

    for (const auto& cardCertAndPin : cardCertAndPinInfos) {
        // Check if the certificate read from the eID matches the certificate provided as argument.
        if (cardCertAndPin.certificate.toDer() == userEidCertificateFromArgs) {
            cardWithCertificateFromArgs = &cardCertAndPin;
        }
    }

    // No eID had the certificate provided as argument.
    if (!cardWithCertificateFromArgs) {
        emit signingCertificateMismatch();
        return;
    }

    if (!cardWithCertificateFromArgs->cardInfo->isSupportedSigningHashAlgorithm(hashAlgo)) {
        THROW(ArgumentFatalError,
              "Electronic ID " + cardWithCertificateFromArgs->cardInfo->name()
                  + " does not support hash algorithm " + std::string(hashAlgo));
    }

    emit singleCertificateReady(origin, *cardWithCertificateFromArgs);
}

QVariantMap Sign::onConfirm(WebEidUI* window, const CardCertificateAndPinInfo& cardCertAndPin)
{
    try {
        pcsc_cpp::byte_vector pin;
        // Reserve space for APDU overhead (5 bytes) + PIN padding (16 bytes) to prevent PIN memory
        // reallocation. The 16-byte limit comes from the max PIN length of 12 bytes across all card
        // implementations in lib/libelectronic-id/src/electronic-ids/pcsc/.
        pin.reserve(5 + 16);
        getPin(pin, *cardCertAndPin.cardInfo, window);
        auto signature = signHash(*cardCertAndPin.cardInfo, std::move(pin), docHash, hashAlgo);
        return {{QStringLiteral("signature"), std::move(signature.first)},
                {QStringLiteral("signatureAlgorithm"), std::move(signature.second)}};

    } catch (const VerifyPinFailed& failure) {
        switch (failure.status()) {
        case electronic_id::VerifyPinFailed::Status::PIN_ENTRY_CANCEL:
        case electronic_id::VerifyPinFailed::Status::PIN_ENTRY_TIMEOUT:
            break;
        case electronic_id::VerifyPinFailed::Status::PIN_ENTRY_DISABLED:
            emit retry(RetriableError::PIN_VERIFY_DISABLED);
            break;
        default:
            emit verifyPinFailed(failure.status(), failure.retries());
        }
        // Retries > 0 means that there are retries remaining,
        // < 0 means that retry count is unknown, == 0 means that the PIN is blocked.
        if (failure.retries() != 0) {
            throw CommandHandlerVerifyPinFailed(failure.what());
        }
        throw;
    }
}

void Sign::connectSignals(const WebEidUI* window)
{
    CertificateReader::connectSignals(window);

    connect(this, &Sign::signingCertificateMismatch, window,
            &WebEidUI::onSigningCertificateMismatch);
    connect(this, &Sign::verifyPinFailed, window, &WebEidUI::onVerifyPinFailed);
}

void Sign::validateAndStoreDocHashAndHashAlgo(const QVariantMap& args)
{
    docHash =
        QByteArray::fromBase64(validateAndGetArgument<QByteArray>(QStringLiteral("hash"), args));

    auto hashAlgoInput = validateAndGetArgument<QString>(QStringLiteral("hashFunction"), args);
    if (hashAlgoInput.size() > 8) {
        THROW(CommandHandlerInputDataError, "hashFunction value is invalid");
    }
    hashAlgo = HashAlgorithm(hashAlgoInput.toStdString());

    if (docHash.length() != int(hashAlgo.hashByteLength())) {
        THROW(CommandHandlerInputDataError,
              std::string(hashAlgo) + " hash must be " + std::to_string(hashAlgo.hashByteLength())
                  + " bytes long, but is " + std::to_string(docHash.length()) + " instead");
    }
}
