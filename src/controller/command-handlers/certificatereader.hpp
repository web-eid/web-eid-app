// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "commandhandler.hpp"

#include <QUrl>

class CertificateReader : public CommandHandler
{
    Q_OBJECT

public:
    explicit CertificateReader(const CommandWithArguments& cmd);

    void run(std::vector<electronic_id::ElectronicID::ptr>&& eids) override;
    void connectSignals(const WebEidUI* window) override;

protected:
    virtual void
    emitCertificatesReady(const std::vector<EidCertificateAndPinInfo>& certAndPinInfos);
    void validateAndStoreOrigin(const QVariantMap& arguments);

    electronic_id::CertificateType certificateType = electronic_id::CertificateType::NONE;
    QUrl origin;
};
