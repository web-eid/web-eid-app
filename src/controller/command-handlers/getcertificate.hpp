// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "certificatereader.hpp"

class GetCertificate : public CertificateReader
{
    Q_OBJECT

public:
    GetCertificate(const CommandWithArguments& cmd);

    QVariantMap onConfirm(WebEidUI* window,
                          const EidCertificateAndPinInfo& certAndPinInfo) override;
};
