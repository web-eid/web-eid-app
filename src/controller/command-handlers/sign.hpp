// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "certificatereader.hpp"

class Sign : public CertificateReader
{
    Q_OBJECT

public:
    explicit Sign(const CommandWithArguments& cmd);

    void connectSignals(const WebEidUI* window) override;
    QVariantMap onConfirm(WebEidUI* window,
                          const EidCertificateAndPinInfo& certAndPinInfo) override;

signals:
    void signingCertificateMismatch();

private:
    void
    emitCertificatesReady(const std::vector<EidCertificateAndPinInfo>& certAndPinInfos) override;
    void validateAndStoreDocHashAndHashAlgo(const QVariantMap& args);

    QByteArray docHash;
    electronic_id::HashAlgorithm hashAlgo;
    QByteArray userEidCertificateFromArgs;
};
