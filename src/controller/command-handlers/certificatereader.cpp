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

using namespace electronic_id;

namespace
{

const QMap<ElectronicID::Type, QString> ICONS {
    {ElectronicID::EstEID, QStringLiteral(":/images/esteid.png")},
    {ElectronicID::FinEID, QStringLiteral(":/images/fineid.png")},
    {ElectronicID::LatEID, QStringLiteral(":/images/lateid.png")},
    {ElectronicID::LitEID, QStringLiteral(":/images/liteid.png")},
};

std::pair<CertificateReader::CerificateAndDer, CertificateAndPinInfo>
getCertificateWithInfo(CardInfo::ptr card, const CertificateType certificateType,
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

    return {{certificate, certificateDer}, {certificateStatus, certInfo, pinInfo}};
}

} // namespace

CertificateReader::CertificateReader(const CommandWithArguments& cmd) : CommandHandler(cmd)
{
    validateAndStoreOrigin(cmd.second);
}

void CertificateReader::run(const std::vector<CardInfo::ptr>& availableCards)
{
    REQUIRE_NOT_EMPTY_CONTAINS_NON_NULL_PTRS(availableCards);
    cards = availableCards;

    const bool isAuthenticate = command.first == CommandType::AUTHENTICATE
        || command.second[QStringLiteral("type")] == QStringLiteral("auth");
    certificateType = isAuthenticate ? CertificateType::AUTHENTICATION : CertificateType::SIGNING;

    std::vector<CertificateAndPinInfo> certInfos;

    for (const auto& card : cards) {
        auto certWithInfo = getCertificateWithInfo(card, certificateType, isAuthenticate);
        certificates.emplace_back(certWithInfo.first);
        certInfos.emplace_back(certWithInfo.second);
    }

    // TODO: check invalid certs, what do they return for subject, issuer etc (probably default
    // values)?
    emit certificatesReady(origin, certInfos);
}

void CertificateReader::connectSignals(const WebEidUI* window)
{
    window->disconnect(this);
    connect(this, &CertificateReader::certificatesReady, window, &WebEidUI::onCertificatesReady);
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

std::tuple<CardInfo::ptr&, const QSslCertificate&, const QByteArray&>
CertificateReader::requireValidCardInfoAndCertificate(const size_t selectedCardIndex)
{
    REQUIRE_NOT_EMPTY_CONTAINS_NON_NULL_PTRS(cards)

    if (selectedCardIndex >= cards.size()) {
        THROW(ProgrammingError,
              "Selected card index " + std::to_string(selectedCardIndex)
                  + " is out of bounds of cards vector size " + std::to_string(cards.size()));
    }
    if (cards.size() != certificates.size()) {
        THROW(ProgrammingError,
              "Cards vector size " + std::to_string(cards.size())
                  + " is not equal to certificates vector size "
                  + std::to_string(certificates.size()));
    }
    for (const auto& certificate : certificates) {
        if (certificate.first.isNull() || certificate.second.isEmpty()) {
            THROW(ProgrammingError,
                  "Invalid certificate in certificate vector, shouldn't happen as it has been "
                  "validated before");
        }
    }

    return {cards[selectedCardIndex], certificates[selectedCardIndex].first,
            certificates[selectedCardIndex].second};
}
