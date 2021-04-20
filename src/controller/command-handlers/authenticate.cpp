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

const auto JWT_BASE64_OPTIONS = QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals;

QByteArray jsonDocToBase64(const QJsonDocument& doc)
{
    return QString(doc.toJson(QJsonDocument::JsonFormat::Compact))
        .toUtf8()
        .toBase64(JWT_BASE64_OPTIONS);
}

QByteArray createAuthenticationToken(const QSslCertificate& certificate,
                                     const QByteArray& certificateDer,
                                     const QString& signatureAlgorithm, const QString& nonce,
                                     const QString& origin,
                                     const QSslCertificate& originCertificate)
{
    const auto tokenHeader = QJsonDocument(QJsonObject {
        {"typ", "JWT"},
        {"alg", signatureAlgorithm},
        {"x5c", QJsonArray({QString(certificateDer.toBase64())})},
    });

    auto tokenPayload = QJsonObject {
        {"iat", QString::number(QDateTime::currentDateTimeUtc().toSecsSinceEpoch())},
        {"exp",
         QString::number(QDateTime::currentDateTimeUtc().addSecs(5 * 60).toSecsSinceEpoch())},
        {"sub", certificate.subjectInfo(QSslCertificate::CommonName)[0]},
        {"nonce", nonce},
        {"iss", QStringLiteral("web-eid app %1").arg(qApp->applicationVersion())},
    };

    auto aud = QJsonArray({origin});
    if (!originCertificate.isNull()) {
        const auto originCertFingerprint =
            QString(originCertificate.digest(QCryptographicHash::Sha256).toHex());
        // urn:cert:sha-256 as per https://tools.ietf.org/id/draft-seantek-certspec-00.html
        aud.append("urn:cert:sha-256:" + originCertFingerprint);
    }
    tokenPayload["aud"] = aud;

    return jsonDocToBase64(tokenHeader) + '.' + jsonDocToBase64(QJsonDocument(tokenPayload));
}

QByteArray signToken(const ElectronicID& eid, const QByteArray& token,
                     const pcsc_cpp::byte_vector& pin)
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

    const auto tokenHashQBytearray = QCryptographicHash::hash(token, hashAlgo);
    const auto tokenHash =
        pcsc_cpp::byte_vector {tokenHashQBytearray.cbegin(), tokenHashQBytearray.cend()};

    const auto signature = eid.signWithAuthKey(pin, tokenHash);

    const auto signatureBase64 =
        QByteArray::fromRawData(reinterpret_cast<const char*>(signature.data()),
                                int(signature.size()))
            .toBase64(JWT_BASE64_OPTIONS);

    return token + '.' + signatureBase64;
}

} // namespace

Authenticate::Authenticate(const CommandWithArguments& cmd) : CertificateReader(cmd)
{
    const auto arguments = cmd.second;

    // nonce, origin, origin-cert
    if (arguments.size() != 3) {
        THROW(CommandHandlerInputDataError,
              "Argument must be '{"
              "\"nonce\": \"<challenge nonce>\", "
              "\"origin\": \"<origin URL>\", "
              "\"origin-cert\": \"<Base64-encoded origin certificate>\"}'");
    }

    nonce = validateAndGetArgument<QString>(QStringLiteral("nonce"), arguments);
    // nonce must contain at least 256 bits of entropy and is usually Base64-encoded, so the
    // required byte length is 44, the length of 32 Base64-encoded bytes.
    if (nonce.length() < 44) {
        THROW(CommandHandlerInputDataError,
              "Challenge nonce argument 'nonce' must be at least 44 characters long");
    }
    if (nonce.length() > 128) {
        THROW(CommandHandlerInputDataError,
              "Challenge nonce argument 'nonce' cannot be longer than 128 characters");
    }
    validateAndStoreOrigin(arguments);
    validateAndStoreOriginCertificate(arguments);
}

QVariantMap Authenticate::onConfirm(WebEidUI* window,
                                    const CardCertificateAndPinInfo& cardCertAndPin)
{
    const auto signatureAlgorithm =
        QString::fromStdString(cardCertAndPin.cardInfo->eid().authSignatureAlgorithm());

    const auto token =
        createAuthenticationToken(cardCertAndPin.certificate, cardCertAndPin.certificateBytesInDer,
                                  signatureAlgorithm, nonce, origin.url(), originCertificate);

    auto pin = getPin(cardCertAndPin.cardInfo->eid().smartcard(), window);

    try {
        const auto signedToken = signToken(cardCertAndPin.cardInfo->eid(), token, pin);

        // Erase the PIN memory.
        // TODO: Use a scope guard. Verify that the buffers are actually zeroed
        // and no copies remain.
        std::fill(pin.begin(), pin.end(), '\0');

        return {{QStringLiteral("auth-token"), QString(signedToken)}};

    } catch (const VerifyPinFailed& failure) {
        emit verifyPinFailed(failure.status(), failure.retries());
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

void Authenticate::validateAndStoreOriginCertificate(const QVariantMap& args)
{
    originCertificate = parseAndValidateCertificate(QStringLiteral("origin-cert"), args, true);
    if (originCertificate.isNull()) {
        return;
    }

    const auto certHostnames = originCertificate.subjectInfo(QSslCertificate::CommonName);
    if (certHostnames.size() != 1) {
        // TODO: add support for multi-domain certificates
        THROW(CommandHandlerInputDataError,
              "Origin certificate does not contain exactly 1 host name (it contains "
                  + std::to_string(certHostnames.size()) + ")");
    }
    if (origin.host() != certHostnames[0]
        // Certificate hostname may be a wildcard, e.g. *.ria.ee, use QDir::match() for glob
        // matching. Origin hostname may be either e.g. www.ria.ee that matches *.ria.ee directly,
        && !QDir::match(certHostnames[0], origin.host())
        // or ria.ee that needs an extra dot prefix to match.
        && !QDir::match(certHostnames[0], '.' + origin.host())) {
        THROW(CommandHandlerInputDataError,
              "Origin host name '" + origin.host().toStdString()
                  + "' does not match origin certificate host name '"
                  + certHostnames[0].toStdString() + "'");
    }
}
