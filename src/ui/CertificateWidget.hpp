#pragma once

#include "certandpininfo.hpp"

#include <QWidget>

struct CertificateInfo;
class QHBoxLayout;
class QListWidget;

class CertificateWidget : public QWidget
{
    Q_OBJECT

public:
    CertificateWidget(QWidget* parent = nullptr);

    void setCertificateInfo(const std::vector<CertificateAndPinInfo>& certificateAndPinInfos);
    size_t selectedCertificateIndex() const;

private:
    void addCertificateListItem(const CertificateInfo& certInfo);

    QHBoxLayout* layout;
    QListWidget* certificateList;
};
