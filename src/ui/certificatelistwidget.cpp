#include "certificatelistwidget.hpp"

#include "certandpininfo.hpp"

#include "electronic-id/electronic-id.hpp"
#include "pcsc-cpp/pcsc-cpp-utils.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>

CertificateListWidget::CertificateListWidget(QWidget* parent) :
    QWidget(parent), layout(new QHBoxLayout(this)), certificateList(new QListWidget(this))
{
    certificateList->setFocusPolicy(Qt::TabFocus);
    layout->addWidget(certificateList);
    layout->setSpacing(10);
    layout->setStretch(1, 1);
    connect(certificateList, &QListWidget::currentItemChanged, this,
            [this]() { emit certificateSelected(); });
}

void CertificateListWidget::setCertificateInfo(
    const std::vector<CardCertificateAndPinInfo>& certAndPinInfos)
{
    certificateList->clear();
    for (const auto& certAndPinInfo : certAndPinInfos) {
        addCertificateListItem(certAndPinInfo);
    }
    resizeHeight();
}

CardCertificateAndPinInfo CertificateListWidget::selectedCertificate() const
{
    QListWidgetItem* currentItem = certificateList->currentItem();
    if (!currentItem) {
        // Should not happen as OK button is disabled until certificateSelected() has been emitted.
        THROW(electronic_id::ProgrammingError, "No certificate selected from the list");
    }
    QVariant certData = currentItem->data(Qt::UserRole);
    if (!certData.isValid() || !certData.canConvert<CardCertificateAndPinInfo>()
        || certData.isNull()) {
        THROW(electronic_id::ProgrammingError, "Unable to retrieve item certificate data");
    }
    return certData.value<CardCertificateAndPinInfo>();
}

void CertificateListWidget::selectFirstRow()
{
    certificateList->setCurrentRow(0);
}

void CertificateListWidget::addCertificateListItem(const CardCertificateAndPinInfo& cardCertPinInfo)
{
    const auto certInfo = cardCertPinInfo.certInfo;
    const auto certType = certInfo.type.isAuthentication() ? QStringLiteral("Authentication")
                                                           : QStringLiteral("Signature");
    auto item = new QListWidgetItem(QIcon(certInfo.icon),
                                    tr("%1: %2\nIssuer: %3\nValid: from %4 to %5")
                                        .arg(certType, certInfo.subject, certInfo.issuer,
                                             certInfo.effectiveDate, certInfo.expiryDate),
                                    certificateList);
    item->setData(Qt::UserRole, QVariant::fromValue(cardCertPinInfo));
}

void CertificateListWidget::resizeHeight()
{
    int totalHeight = 0;
    for (int i = 0; i < certificateList->count(); ++i) {
        totalHeight += certificateList->sizeHintForRow(0);
    }
    certificateList->setFixedHeight(totalHeight + 1);
}
