#pragma once

#include "certandpininfo.hpp"

#include <QListWidget>

struct CertificateInfo;

class CertificateListWidget : public QListWidget
{
    Q_OBJECT

public:
    CertificateListWidget(QWidget* parent = nullptr);

    void setCertificateInfo(const std::vector<CardCertificateAndPinInfo>& certificateAndPinInfos);
    CardCertificateAndPinInfo selectedCertificate() const;

signals:
    void certificateSelected();

private:
    void addCertificateListItem(const CardCertificateAndPinInfo& cardCertPinInfo);
    void resizeHeight();
};
