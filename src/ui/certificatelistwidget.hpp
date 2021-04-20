#pragma once

#include "certandpininfo.hpp"

#include <QWidget>

struct CertificateInfo;
class QHBoxLayout;
class QListWidget;

class CertificateListWidget : public QWidget
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

    QHBoxLayout* layout;
    QListWidget* certificateList;
};
