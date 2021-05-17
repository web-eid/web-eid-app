#pragma once

#include <QListWidget>

struct CardCertificateAndPinInfo;

class CertificateListWidget : public QListWidget
{
    Q_OBJECT

public:
    CertificateListWidget(QWidget* parent = nullptr);

    void setCertificateInfo(const std::vector<CardCertificateAndPinInfo>& certificateAndPinInfos);
    CardCertificateAndPinInfo selectedCertificate() const;
    void selectFirstRow();

signals:
    void certificateSelected();

private:
    void addCertificateListItem(const CardCertificateAndPinInfo& cardCertPinInfo);
    void resizeHeight();
};
