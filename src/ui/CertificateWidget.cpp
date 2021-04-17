#include "CertificateWidget.hpp"

#include "certandpininfo.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>

CertificateWidget::CertificateWidget(QWidget* parent) :
    QWidget(parent), layout(new QHBoxLayout(this)), certificateList(new QListWidget(this))
{
    certificateList->setFocusPolicy(Qt::TabFocus);
    layout->addWidget(certificateList);
    layout->setSpacing(10);
    layout->setStretch(1, 1);
}

void CertificateWidget::setCertificateInfo(
    const std::vector<CertificateAndPinInfo>& certAndPinInfos)
{
    for (const auto& certAndPinInfo : certAndPinInfos) {
        addCertificateListItem(certAndPinInfo.certInfo);
    }
}

size_t CertificateWidget::selectedCertificateIndex() const
{
    return size_t(certificateList->currentRow());
}

void CertificateWidget::addCertificateListItem(const CertificateInfo& certInfo)
{
    const auto certType = certInfo.type.isAuthentication() ? QStringLiteral("Authentication")
                                                           : QStringLiteral("Signature");
    new QListWidgetItem(QIcon(certInfo.icon),
                        tr("%1: %2\nIssuer: %3\nValid: from %4 to %5")
                            .arg(certType, certInfo.subject, certInfo.issuer,
                                 certInfo.effectiveDate, certInfo.expiryDate),
                        certificateList);
}
