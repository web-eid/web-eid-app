// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "qeid.hpp"

#include <QSslCertificate>

struct CertificateInfo
{
    electronic_id::CertificateType type = electronic_id::CertificateType::NONE;

    bool isExpired = false;
    bool notEffective = false;
    QString subject;
};

struct PinInfo
{
    using PinMinMaxLength = std::pair<uint8_t, uint8_t>;
    using PinRetriesCount = std::pair<int8_t, int8_t>;

    PinMinMaxLength pinMinMaxLength {0, 0};
    PinRetriesCount pinRetriesCount {0, -1};
    bool readerHasPinPad = false;
    constexpr bool pinIsBlocked() const { return pinRetriesCount.first == 0; }

    static constexpr int PIN_PAD_PIN_ENTRY_TIMEOUT = pcsc_cpp::PIN_PAD_PIN_ENTRY_TIMEOUT;
};

struct EidCertificateAndPinInfo
{
    electronic_id::ElectronicID::ptr eid;
    QByteArray certificateBytesInDer;
    QSslCertificate certificate {};
    CertificateInfo certInfo;
    PinInfo pinInfo;
    bool pin1Active = true;
    bool pin2Active = true;
};

Q_DECLARE_METATYPE(EidCertificateAndPinInfo)
