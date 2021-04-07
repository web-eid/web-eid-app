#include "CertificateWidget.hpp"

#include "certandpininfo.hpp"

#include <QHBoxLayout>
#include <QLabel>

CertificateWidget::CertificateWidget(QWidget *parent)
    : QWidget(parent),
    layout(new QHBoxLayout(this)),
    icon(new QLabel(this)),
    label(new QLabel(this))
{
    label->setFocusPolicy(Qt::TabFocus);
    label->setWordWrap(true);
    layout->addWidget(icon);
    layout->addWidget(label);
    layout->setSpacing(10);
    layout->setStretch(1, 1);
}

CertificateWidget::CertificateWidget(const CertificateInfo &certInfo, QWidget *parent): CertificateWidget(parent) {
    setCertificateInfo(certInfo);
}

void CertificateWidget::setCertificateInfo(const CertificateInfo &certInfo)
{
    const auto certType = certInfo.type.isAuthentication() ? QStringLiteral("Authentication")
                                                           : QStringLiteral("Signature");
    icon->setPixmap(certInfo.icon);
    label->setText(tr("%1: %2\nIssuer: %3\nValid: from %4 to %5")
                       .arg(certType, certInfo.subject, certInfo.issuer,
                            certInfo.effectiveDate, certInfo.expiryDate));
}
