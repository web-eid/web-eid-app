/*
 * Copyright (c) 2020-2023 Estonian Information System Authority
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

#include "authenticate.hpp"

#include "signauthutils.hpp"

#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QDir>

#include <map>

using namespace electronic_id;

namespace
{

// Use common base64-encoding defaults.
constexpr auto BASE64_OPTIONS = QByteArray::Base64Encoding | QByteArray::KeepTrailingEquals;

QVariantMap createAuthenticationToken(const QString& signatureAlgorithm,
                                      const QByteArray& certificateDer, const QByteArray& signature)
{
    return QVariantMap {
        {"unverifiedCertificate", QString(certificateDer.toBase64(BASE64_OPTIONS))},
        {"algorithm", signatureAlgorithm},
        {"signature", QString(signature)},
        {"format", QStringLiteral("web-eid:1.0")},
        {"appVersion",
         QStringLiteral("https://web-eid.eu/web-eid-app/releases/%1")
             .arg(qApp->applicationVersion())},
    };
}

QByteArray createSignature(const QString& origin, const QString& challengeNonce,
                           const ElectronicID& eid, const pcsc_cpp::byte_vector& pin)
{
    static const auto SIGNATURE_ALGO_TO_HASH =
        std::map<JsonWebSignatureAlgorithm, QCryptographicHash::Algorithm> {
            {JsonWebSignatureAlgorithm::RS256, QCryptographicHash::Sha256},
            {JsonWebSignatureAlgorithm::PS256, QCryptographicHash::Sha256},
            {JsonWebSignatureAlgorithm::ES384, QCryptographicHash::Sha384},
        };

    if (!SIGNATURE_ALGO_TO_HASH.count(eid.authSignatureAlgorithm())) {
        THROW(ProgrammingError,
              "Hash algorithm mapping missing for signature algorithm "
                  + std::string(eid.authSignatureAlgorithm()));
    }

    const auto hashAlgo = SIGNATURE_ALGO_TO_HASH.at(eid.authSignatureAlgorithm());

    // Take the hash of the origin and nonce to ensure field separation.
    const auto originHash = QCryptographicHash::hash(origin.toUtf8(), hashAlgo);
    const auto challengeNonceHash = QCryptographicHash::hash(challengeNonce.toUtf8(), hashAlgo);

    // The value that is signed is hash(origin)+hash(challenge).
    const auto hashToBeSignedQBytearray =
        QCryptographicHash::hash(originHash + challengeNonceHash, hashAlgo);
    const auto hashToBeSigned =
        pcsc_cpp::byte_vector {hashToBeSignedQBytearray.cbegin(), hashToBeSignedQBytearray.cend()};

    const auto signature = eid.signWithAuthKey(pin, hashToBeSigned);

    return QByteArray::fromRawData(reinterpret_cast<const char*>(signature.data()),
                                   int(signature.size()))
        .toBase64(BASE64_OPTIONS);
}

} // namespace

Authenticate::Authenticate(const CommandWithArguments& cmd) : CertificateReader(cmd)
{
    const auto arguments = cmd.second;
    requireArgumentsAndOptionalLang({"challengeNonce", "origin"}, arguments,
                                    "\"challengeNonce\": \"<challenge nonce>\", "
                                    "\"origin\": \"<origin URL>\"");

    challengeNonce = validateAndGetArgument<QString>(QStringLiteral("challengeNonce"), arguments);
    // nonce must contain at least 256 bits of entropy and is usually Base64-encoded, so the
    // required byte length is 44, the length of 32 Base64-encoded bytes.
    if (challengeNonce.length() < 44) {
        THROW(CommandHandlerInputDataError,
              "Challenge nonce argument 'challengeNonce' must be at least 44 characters long");
    }
    if (challengeNonce.length() > 128) {
        THROW(CommandHandlerInputDataError,
              "Challenge nonce argument 'challengeNonce' cannot be longer than 128 characters");
    }
    validateAndStoreOrigin(arguments);
}

QVariantMap Authenticate::onConfirm(WebEidUI* window,
                                    const CardCertificateAndPinInfo& cardCertAndPin)
{
    const auto signatureAlgorithm =
        QString::fromStdString(cardCertAndPin.cardInfo->eid().authSignatureAlgorithm());

    auto pin = getPin(cardCertAndPin.cardInfo->eid().smartcard(), window);

    try {
        const auto signature =
            createSignature(origin.url(), challengeNonce, cardCertAndPin.cardInfo->eid(), pin);

        // Erase the PIN memory.
        // TODO: Use a scope guard. Verify that the buffers are actually zeroed and no copies
        // remain.
        std::fill(pin.begin(), pin.end(), '\0');

        return createAuthenticationToken(signatureAlgorithm, cardCertAndPin.certificateBytesInDer,
                                         signature);

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
        if (failure.retries() > 0) {
            throw CommandHandlerVerifyPinFailed(failure.what());
        }
        throw;
    }
}

void Authenticate::connectSignals(const WebEidUI* window)
{
    CertificateReader::connectSignals(window);

    connect(this, &Authenticate::verifyPinFailed, window, &WebEidUI::onVerifyPinFailed);
}
