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

#include "certificatereader.hpp"

#include "signauthutils.hpp"
#include "utils.hpp"
#include "magic_enum/magic_enum.hpp"

using namespace electronic_id;

namespace
{

enum class CertificateStatus { VALID, INVALID, NOT_YET_ACTIVE, EXPIRED };

QString certificateStatusToString(const CertificateStatus status)
{
    return QString::fromStdString(std::string(magic_enum::enum_name(status)));
}

const QMap<ElectronicID::Type, QString> ICONS {
    {ElectronicID::EstEID, QStringLiteral(":/images/esteid.png")},
    {ElectronicID::FinEID, QStringLiteral(":/images/fineid.png")},
    {ElectronicID::LatEID, QStringLiteral(":/images/lateid.png")},
    {ElectronicID::LitEID, QStringLiteral(":/images/liteid.png")},
};

std::pair<CertificateStatus, CardCertificateAndPinInfo>
getCertificateWithStatusAndInfo(CardInfo::ptr card, const CertificateType certificateType,
                                const bool isAuthenticate)
{
    const auto certificateBytes = card->eid().getCertificate(certificateType);

    auto certificateDer = QByteArray(reinterpret_cast<const char*>(certificateBytes.data()),
                                     int(certificateBytes.size()));
    auto certificate = QSslCertificate(certificateDer, QSsl::Der);
    if (certificate.isNull()) {
        THROW(SmartCardChangeRequiredError,
              "Invalid certificate returned by electronic ID " + card->eid().name());
    }

    auto certificateStatus = CertificateStatus::VALID;

    if (certificate.isNull()) {
        certificateStatus = CertificateStatus::INVALID;
    }
    if (certificate.effectiveDate() > QDateTime::currentDateTimeUtc()) {
        certificateStatus = CertificateStatus::NOT_YET_ACTIVE;
    }
    if (certificate.expiryDate() < QDateTime::currentDateTimeUtc()) {
        certificateStatus = CertificateStatus::EXPIRED;
    }

    auto certInfo = CertificateInfo {certificateType,
                                     ICONS.value(card->eid().type()),
                                     certificate.subjectInfo(QSslCertificate::CommonName).join(' '),
                                     certificate.issuerInfo(QSslCertificate::CommonName).join(' '),
                                     certificate.effectiveDate().date().toString(Qt::ISODate),
                                     certificate.expiryDate().date().toString(Qt::ISODate)};
    auto pinInfo = PinInfo {
        isAuthenticate ? card->eid().authPinMinMaxLength() : card->eid().signingPinMinMaxLength(),
        isAuthenticate ? card->eid().authPinRetriesLeft() : card->eid().signingPinRetriesLeft(),
        card->eid().smartcard().readerHasPinPad()};
    if (pinInfo.pinRetriesCount.first == 0) {
        pinInfo.pinIsBlocked = true;
    }

    return {certificateStatus, {card, certificateDer, certificate, certInfo, pinInfo}};
}

} // namespace

CertificateReader::CertificateReader(const CommandWithArguments& cmd) : CommandHandler(cmd)
{
    validateAndStoreOrigin(cmd.second);
}

void CertificateReader::run(const std::vector<CardInfo::ptr>& cards)
{
    REQUIRE_NOT_EMPTY_CONTAINS_NON_NULL_PTRS(cards);

    const bool isAuthenticate = command.first == CommandType::AUTHENTICATE
        || command.second[QStringLiteral("type")] == QStringLiteral("auth");
    certificateType = isAuthenticate ? CertificateType::AUTHENTICATION : CertificateType::SIGNING;

    std::vector<CardCertificateAndPinInfo> certInfos;

    for (const auto& card : cards) {
        auto certStatusAndInfo =
            getCertificateWithStatusAndInfo(card, certificateType, isAuthenticate);
        // Omit invalid certificates.
        if (certStatusAndInfo.first != CertificateStatus::VALID) {
            qWarning() << "The" << (isAuthenticate ? "authentication" : "signing")
                       << "certificate status is not valid:"
                       << certificateStatusToString(certStatusAndInfo.first);
            continue;
        }
        certInfos.emplace_back(certStatusAndInfo.second);
    }

    if (certInfos.empty()) {
        emit retry(RetriableError::NO_VALID_CERTIFICATE_AVAILABLE);
    } else {
        emitCertificatesReady(certInfos);
    }
}

void CertificateReader::connectSignals(const WebEidUI* window)
{
    window->disconnect(this);
    connect(this, &CertificateReader::multipleCertificatesReady, window,
            &WebEidUI::onMultipleCertificatesReady);
    connect(this, &CertificateReader::singleCertificateReady, window,
            &WebEidUI::onSingleCertificateReady);
}

void CertificateReader::emitCertificatesReady(
    const std::vector<CardCertificateAndPinInfo>& certInfos)
{
    if (certInfos.size() == 1) {
        emit singleCertificateReady(origin, certInfos[0]);
    } else {
        emit multipleCertificatesReady(origin, certInfos);
    }
}

void CertificateReader::validateAndStoreOrigin(const QVariantMap& arguments)
{
    const auto originStr = validateAndGetArgument<QString>(QStringLiteral("origin"), arguments);
    if (originStr.size() > 255) {
        THROW(CommandHandlerInputDataError, "origin length cannot exceed 255 characters");
    }

    origin = QUrl(originStr, QUrl::ParsingMode::StrictMode);

    if (!origin.isValid()) {
        THROW(CommandHandlerInputDataError, "origin is not a valid URL");
    }
    if (origin.isRelative() || !origin.path().isEmpty() || origin.hasQuery()
        || origin.hasFragment()) {
        THROW(CommandHandlerInputDataError, "origin is not in <scheme>://<host>[:<port>] format");
    }
    if (origin.scheme() != QStringLiteral("https") && origin.scheme() != QStringLiteral("wss")) {
        THROW(CommandHandlerInputDataError, "origin scheme has to be https or wss");
    }
}
