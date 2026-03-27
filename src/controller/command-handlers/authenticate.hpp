// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "certificatereader.hpp"

class Authenticate : public CertificateReader
{
    Q_OBJECT

public:
    explicit Authenticate(const CommandWithArguments& cmd);

    void connectSignals(const WebEidUI* window) override;
    QVariantMap onConfirm(WebEidUI* window,
                          const EidCertificateAndPinInfo& certAndPinInfo) override;

private:
    QString challengeNonce;
};
