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

namespace
{

QPair<QString, QVariantMap> signHash(const electronic_id::ElectronicID& eid,
                                     const pcsc_cpp::byte_vector& pin, const QByteArray& docHash,
                                     const electronic_id::HashAlgorithm hashAlgo)
{
    const auto hashBytes = pcsc_cpp::byte_vector {docHash.begin(), docHash.end()};
    const auto signature = eid.signWithSigningKey(pin, hashBytes, hashAlgo);

    const auto signatureBase64 =
        QByteArray::fromRawData(reinterpret_cast<const char*>(signature.first.data()),
                                int(signature.first.size()))
            .toBase64();

    return {signatureBase64, signatureAlgoToVariantMap(signature.second)};
}

// Convert the hash into fingerprint quadruplets separated by blanks.
QString convertToFingerprintFormat(const QByteArray& docHash)
{
    const auto fingerprint = QString(docHash.toHex(':'));
    QStringList result;
    for (int i = 0; i < fingerprint.length(); i += 4) {
        result.append(fingerprint.section(':', i, i + 3));
    }
    return result.join(' ');
}

} // namespace

Sign::Sign(const CommandWithArguments& cmd) : CertificateReader(cmd)
{
    const auto arguments = cmd.second;

    // FIXME: implement argument validation with custom exceptions that are sent back up.
    // doc-hash, origin
    if (arguments.size() != 4) {
        throw std::invalid_argument("sign: argument must be '{"
                                    "\"doc-hash\": \"<Base64-encoded document hash>\", "
                                    "\"hash-algo\": \"<the hash algorithm that was used for "
                                    "computing 'doc-hash', any of "
                                    + electronic_id::HashAlgorithm::allSupportedAlgorithmNames()
                                    + ">\", \"user-eid-cert\": \"<Base64-encoded user eID "
                                      "certificate previously retrieved with get-cert>\", "
                                      "\"origin\": \"<origin URL>\"}'");
    }

    validateAndStoreDocHashAndHashAlgo(arguments);

    userEidCertificateFromArgs =
        parseAndValidateCertificate(QStringLiteral("user-eid-cert"), arguments);
    validateAndStoreOrigin(arguments);
}

void Sign::run(electronic_id::CardInfo::ptr _cardInfo)
{
    requireNonNull(_cardInfo, "_cardInfo", "Sign::run");
    cardInfo = _cardInfo;

    if (!cardInfo->eid().isSupportedSigningHashAlgorithm(hashAlgo)) {
        // FIXME: implement argument validation with custom exceptions that are sent back up.
        throw std::invalid_argument("Sign::run(): inserted electronic ID " + cardInfo->eid().name()
                                    + " does not support hash algorithm " + std::string(hashAlgo));
    }

    CertificateReader::run(cardInfo);

    // Assure that the certificate read from the eID card matches the certificate provided as
    // argument.
    if (certificate.digest(QCryptographicHash::Sha1)
        == userEidCertificateFromArgs.digest(QCryptographicHash::Sha1)) {
        emit documentHashReady(convertToFingerprintFormat(docHash));
    } else {
        emit certificateHashMismatch();
    }
}

QVariantMap Sign::onConfirm(WebEidUI* window)
{
    requireNonNull(cardInfo, "cardInfo", "Sign::onConfirm");

    if (certificate.isNull()) {
        throw electronic_id::Error("Authenticate::onConfirm(): invalid certificate");
    }

    auto pin = getPin(window, QStringLiteral("signingPinInput"));

    try {
        const auto signature = signHash(cardInfo->eid(), pin, docHash, hashAlgo);

        // Erase PIN memory.
        // FIXME: Use a scope guard. Discuss if zero-filling is OK or is random better.
        std::fill(pin.begin(), pin.end(), '\0');

        return {{QStringLiteral("signature"), signature.first},
                {QStringLiteral("signature-algo"), signature.second}};

    } catch (const electronic_id::VerifyPinFailed& failure) {
        emit verifyPinFailed(failure.status(), failure.retries());
        throw CommandHandlerRetriableError(failure.what());
    }
}

void Sign::connectSignals(WebEidUI* window)
{
    // TODO: DRY with Authenticate?
    CertificateReader::connectSignals(window);

    connect(this, &Sign::documentHashReady, window, &WebEidUI::onDocumentHashReady);
    connect(this, &Sign::certificateHashMismatch, window,
            &WebEidUI::onSigningCertificateHashMismatch);
    connect(this, &Sign::verifyPinFailed, window, &WebEidUI::onVerifyPinFailed);
}

void Sign::validateAndStoreDocHashAndHashAlgo(const QVariantMap& args)
{
    docHash = QByteArray::fromBase64(
        validateAndGetArgument<QByteArray>(QStringLiteral("doc-hash"), args));

    QString hashAlgoInput = validateAndGetArgument<QString>(QStringLiteral("hash-algo"), args);
    if (hashAlgoInput.size() > 8) {
        throw std::invalid_argument("sign: hash-algo value is invalid");
    }
    hashAlgo = electronic_id::HashAlgorithm(hashAlgoInput.toStdString());

    if (docHash.length() != int(hashAlgo.hashByteLength())) {
        throw std::invalid_argument("sign: " + std::string(hashAlgo) + " hash must be "
                                    + std::to_string(hashAlgo.hashByteLength())
                                    + " bytes long, but is " + std::to_string(docHash.length())
                                    + " instead");
    }
}
