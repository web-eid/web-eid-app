#include "certificatelistwidget.hpp"

#include "certandpininfo.hpp"

#include "pcsc-cpp/pcsc-cpp-utils.hpp"

CertificateListWidget::CertificateListWidget(QWidget* parent) : QListWidget(parent)
{
    connect(this, &QListWidget::currentItemChanged, this,
            &CertificateListWidget::certificateSelected);
}

void CertificateListWidget::setCertificateInfo(
    const std::vector<CardCertificateAndPinInfo>& certAndPinInfos)
{
    if (certAndPinInfos.empty()) {
        // Should not happen as certAndPinInfos is pre-validated.
        THROW(electronic_id::ProgrammingError, "certAndPinInfos is empty");
    }
    clear();
    for (const auto& certAndPinInfo : certAndPinInfos) {
        addCertificateListItem(certAndPinInfo);
    }
    resizeHeight();
}

CardCertificateAndPinInfo CertificateListWidget::selectedCertificate() const
{
    if (!currentItem()) {
        // Should not happen as OK button is disabled until certificateSelected() has been emitted.
        THROW(electronic_id::ProgrammingError, "No certificate selected from the list");
    }
    QVariant certData = currentItem()->data(Qt::UserRole);
    if (!certData.isValid() || !certData.canConvert<CardCertificateAndPinInfo>()
        || certData.isNull()) {
        THROW(electronic_id::ProgrammingError, "Unable to retrieve item certificate data");
    }
    return certData.value<CardCertificateAndPinInfo>();
}

void CertificateListWidget::addCertificateListItem(const CardCertificateAndPinInfo& cardCertPinInfo)
{
    const auto certInfo = cardCertPinInfo.certInfo;
    auto item = new QListWidgetItem(
        QIcon(QStringLiteral(":/images/id-card.svg")),
        tr("%1\nIssuer: %2\nValid: from %3 to %4")
            .arg(certInfo.subject, certInfo.issuer, certInfo.effectiveDate, certInfo.expiryDate),
        this);
    item->setData(Qt::UserRole, QVariant::fromValue(cardCertPinInfo));
}

void CertificateListWidget::resizeHeight()
{
    int totalHeight = 0;
    for (int i = 0; i < count(); ++i) {
        totalHeight += sizeHintForRow(0);
    }
    setFixedHeight(totalHeight + 1 + count() * 6);
}
