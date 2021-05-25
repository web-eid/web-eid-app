#include "certificatelistwidget.hpp"

#include "certandpininfo.hpp"

#include "pcsc-cpp/pcsc-cpp-utils.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

CertificateWidget::CertificateWidget(QWidget* parent) :
    QWidget(parent), icon(new QLabel(this)), info(new QLabel(this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(icon);
    layout->addWidget(info, 1);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(20);
}

CertificateWidget::CertificateWidget(const CardCertificateAndPinInfo& cardCertPinInfo,
                                     QWidget* parent) :
    QWidget(parent)
{
    setCertificateInfo(cardCertPinInfo);
}

void CertificateWidget::paintEvent(QPaintEvent* /*event*/)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    // Applies style sheet styling to the custom widget.
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void CertificateWidget::setCertificateInfo(const CardCertificateAndPinInfo& cardCertPinInfo)
{
    const auto certInfo = cardCertPinInfo.certInfo;
    icon->setPixmap(QStringLiteral(":/images/id-card.svg"));
    info->setText(
        tr("<b>%1</b><br />Issuer: %2<br />Valid: from %3 to %4")
            .arg(certInfo.subject, certInfo.issuer, certInfo.effectiveDate, certInfo.expiryDate));
    info->setFocusPolicy(Qt::TabFocus);
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
    QListWidgetItem* item = new QListWidgetItem(this);
    item->setData(Qt::UserRole, QVariant::fromValue(cardCertPinInfo));
    setItemWidget(item, new CertificateWidget(cardCertPinInfo, this));
}

void CertificateListWidget::resizeHeight()
{
    int totalHeight = 0;
    for (int i = 0; i < count(); ++i) {
        totalHeight += sizeHintForRow(0);
    }
    setFixedHeight(totalHeight + 1 + count() * 6);
}
