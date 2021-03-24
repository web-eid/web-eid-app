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
        {"iat", QString::number(QDateTime::currentDateTime().toTime_t())},
        {"exp", QString::number(QDateTime::currentDateTime().addSecs(5 * 60).toTime_t())},
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

QByteArray signToken(const electronic_id::ElectronicID& eid, const QByteArray& token,
                     const pcsc_cpp::byte_vector& pin)
{
    static const auto SIGNATURE_ALGO_TO_HASH =
        std::map<electronic_id::JsonWebSignatureAlgorithm, QCryptographicHash::Algorithm> {
            {electronic_id::JsonWebSignatureAlgorithm::RS256, QCryptographicHash::Sha256},
            {electronic_id::JsonWebSignatureAlgorithm::PS256, QCryptographicHash::Sha256},
            {electronic_id::JsonWebSignatureAlgorithm::ES384, QCryptographicHash::Sha384},
        };

    if (!SIGNATURE_ALGO_TO_HASH.count(eid.authSignatureAlgorithm())) {
        throw std::logic_error("Authenticate::onConfirm():signToken(): "
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

    // FIXME: implement argument validation with custom exceptions that are sent back up.
    // nonce, origin, origin-cert
    if (arguments.size() != 3) {
        throw std::invalid_argument("authenticate: argument must be '{"
                                    "\"nonce\": \"<challenge nonce>\", "
                                    "\"origin\": \"<origin URL>\", "
                                    "\"origin-cert\": \"<Base64-encoded origin certificate>\"}'");
    }

    nonce = validateAndGetArgument<QString>(QStringLiteral("nonce"), arguments);
    // TODO: nonce must contain at least 256 bits of entropy, but what should the required byte
    // length be considering encoding?
    if (nonce.length() < 32) {
        throw std::invalid_argument("authenticate: challenge nonce argument 'nonce' must be "
                                    "at least 32 characters long");
    }
    if (nonce.length() > 128) {
        throw std::invalid_argument("authenticate: challenge nonce argument 'nonce' must be "
                                    "at maximum 128 characters long");
    }
    validateAndStoreOrigin(arguments);
    validateAndStoreCertificate(arguments);
}

QVariantMap Authenticate::onConfirm(WebEidUI* window)
{
    if (certificate.isNull()) {
        throw electronic_id::Error("Authenticate::onConfirm(): invalid certificate");
    }
    if (!cardInfo) {
        throw std::logic_error("Authenticate::onConfirm(): null cardInfo");
    }

    const auto signatureAlgorithm =
        QString::fromStdString(cardInfo->eid().authSignatureAlgorithm());

    const auto token = createAuthenticationToken(certificate, certificateDer, signatureAlgorithm,
                                                 nonce, origin.url(), originCertificate);

    auto pin = getPin(window, QStringLiteral("authenticationPinInput"));

    try {
        const auto signedToken = signToken(cardInfo->eid(), token, pin);

        // Erase PIN memory.
        // FIXME: Use a scope guard. Discuss if zero-filling is OK or is random better.
        std::fill(pin.begin(), pin.end(), '\0');

        return {{QStringLiteral("auth-token"), QString(signedToken)}};

    } catch (const electronic_id::VerifyPinFailed& failure) {
        emit verifyPinFailed(failure.status(), failure.retries());
        if(failure.retries() > 0) {
            throw CommandHandlerRetriableError(failure.what());
        }
        throw;
    }
}

void Authenticate::connectSignals(WebEidUI* window)
{
    CertificateReader::connectSignals(window);

    connect(this, &Authenticate::verifyPinFailed, window, &WebEidUI::onVerifyPinFailed);
}

void Authenticate::validateAndStoreCertificate(const QVariantMap& args)
{
    originCertificate = parseAndValidateCertificate(QStringLiteral("origin-cert"), args, true);
    if (originCertificate.isNull()) {
        return;
    }

    const auto certHostnames = originCertificate.subjectInfo(QSslCertificate::CommonName);
    if (certHostnames.size() != 1) {
        // FIXME: add support for multi-domain certificates
        throw std::invalid_argument("authenticate: origin certificate does not contain "
                                    "exactly 1 host name (it contains "
                                    + std::to_string(certHostnames.size()) + ")");
    }
    if (origin.host() != certHostnames[0]
        // Certificate hostname may be a wildcard, e.g. *.ria.ee, use QDir::match() for glob
        // matching. Origin hostname may be either e.g. www.ria.ee that matches *.ria.ee directly,
        && !QDir::match(certHostnames[0], origin.host())
        // or ria.ee that needs an extra dot prefix to match.
        && !QDir::match(certHostnames[0], '.' + origin.host())) {
        throw std::invalid_argument("authenticate: origin host name '" + origin.host().toStdString()
                                    + "' does not match origin certificate host name '"
                                    + certHostnames[0].toStdString() + "'");
    }
}
