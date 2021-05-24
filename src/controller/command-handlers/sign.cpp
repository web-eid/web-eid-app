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

#include "sign.hpp"

#include "signauthutils.hpp"
#include "utils.hpp"

using namespace electronic_id;

namespace
{

QPair<QString, QVariantMap> signHash(const ElectronicID& eid, const pcsc_cpp::byte_vector& pin,
                                     const QByteArray& docHash, const HashAlgorithm hashAlgo)
{
    const auto hashBytes = pcsc_cpp::byte_vector {docHash.begin(), docHash.end()};
    const auto signature = eid.signWithSigningKey(pin, hashBytes, hashAlgo);

    const auto signatureBase64 =
        QByteArray::fromRawData(reinterpret_cast<const char*>(signature.first.data()),
                                int(signature.first.size()))
            .toBase64();

    return {signatureBase64, signatureAlgoToVariantMap(signature.second)};
}

} // namespace

Sign::Sign(const CommandWithArguments& cmd) : CertificateReader(cmd)
{
    const auto arguments = cmd.second;

    requireArgumentsAndOptionalLang(
        {"doc-hash", "hash-algo", "user-eid-cert", "origin"}, arguments,
        "\"doc-hash\": \"<Base64-encoded document hash>\", "
        "\"hash-algo\": \"<the hash algorithm that was used for computing 'doc-hash', any of "
            + HashAlgorithm::allSupportedAlgorithmNames()
            + ">\", \"user-eid-cert\": \"<Base64-encoded user eID certificate previously "
              "retrieved with get-cert>\", "
              "\"origin\": \"<origin URL>\"");

    validateAndStoreDocHashAndHashAlgo(arguments);

    userEidCertificateFromArgs =
        parseAndValidateCertificate(QStringLiteral("user-eid-cert"), arguments);
    validateAndStoreOrigin(arguments);
}

void Sign::emitCertificatesReady(const std::vector<CardCertificateAndPinInfo>& cardCertAndPinInfos)
{
    const CardCertificateAndPinInfo* cardWithCertificateFromArgs = nullptr;

    for (const auto& cardCertAndPin : cardCertAndPinInfos) {
        // Check if the certificate read from the eID matches the certificate provided as argument.
        if (cardCertAndPin.certificate.digest(QCryptographicHash::Sha256)
            == userEidCertificateFromArgs.digest(QCryptographicHash::Sha256)) {
            cardWithCertificateFromArgs = &cardCertAndPin;
        }
    }

    // No eID had the certificate provided as argument.
    if (!cardWithCertificateFromArgs) {
        const auto certSubject =
            userEidCertificateFromArgs.subjectInfo(QSslCertificate::CommonName).join(' ');
        emit certificateNotFound(certSubject);
        return;
    }

    if (!cardWithCertificateFromArgs->cardInfo->eid().isSupportedSigningHashAlgorithm(hashAlgo)) {
        THROW(ArgumentFatalError,
              "Electronic ID " + cardWithCertificateFromArgs->cardInfo->eid().name()
                  + " does not support hash algorithm " + std::string(hashAlgo));
    }

    emit singleCertificateReady(origin, *cardWithCertificateFromArgs);
}

QVariantMap Sign::onConfirm(WebEidUI* window, const CardCertificateAndPinInfo& cardCertAndPin)
{
    auto pin = getPin(cardCertAndPin.cardInfo->eid().smartcard(), window);

    try {
        const auto signature = signHash(cardCertAndPin.cardInfo->eid(), pin, docHash, hashAlgo);

        // Erase PIN memory.
        // TODO: Use a scope guard. Verify that the buffers are actually zeroed
        // and no copies remain.
        std::fill(pin.begin(), pin.end(), '\0');

        return {{QStringLiteral("signature"), signature.first},
                {QStringLiteral("signature-algo"), signature.second}};

    } catch (const VerifyPinFailed& failure) {
        emit verifyPinFailed(failure.status(), failure.retries());
        if (failure.retries() > 0) {
            throw CommandHandlerVerifyPinFailed(failure.what());
        }
        throw;
    }
}

void Sign::connectSignals(const WebEidUI* window)
{
    CertificateReader::connectSignals(window);

    connect(this, &Sign::certificateNotFound, window, &WebEidUI::onCertificateNotFound);
    connect(this, &Sign::verifyPinFailed, window, &WebEidUI::onVerifyPinFailed);
}

void Sign::validateAndStoreDocHashAndHashAlgo(const QVariantMap& args)
{
    docHash = QByteArray::fromBase64(
        validateAndGetArgument<QByteArray>(QStringLiteral("doc-hash"), args));

    QString hashAlgoInput = validateAndGetArgument<QString>(QStringLiteral("hash-algo"), args);
    if (hashAlgoInput.size() > 8) {
        THROW(CommandHandlerInputDataError, "hash-algo value is invalid");
    }
    hashAlgo = HashAlgorithm(hashAlgoInput.toStdString());

    if (docHash.length() != int(hashAlgo.hashByteLength())) {
        THROW(CommandHandlerInputDataError,
              std::string(hashAlgo) + " hash must be " + std::to_string(hashAlgo.hashByteLength())
                  + " bytes long, but is " + std::to_string(docHash.length()) + " instead");
    }
}
