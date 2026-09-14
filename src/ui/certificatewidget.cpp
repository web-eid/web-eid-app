// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "certificatewidget.hpp"

#include "application.hpp"

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStyleOption>

namespace
{

inline QString displayInRed(const QString& text)
{
    return QStringLiteral("<span style=\"color: #CD2541\">%1</span>").arg(text);
}

} // namespace

// We use two separate widgets, CertificateWidget and CertificateButton, for accessibility, to
// support screen readers.

CertificateWidgetInfo::CertificateWidgetInfo(QWidget* self) :
    icon(new QLabel(self)), info(new QLabel(self)),
    warn(new QLabel(CertificateWidget::tr("Pin locked"), self))
{
    warn->setObjectName(QStringLiteral("warn"));
    warn->hide();
    auto* layout = new QHBoxLayout(self);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(10);
    layout->addWidget(icon);
    layout->addWidget(info, 1);
    auto* warnLayout = new QHBoxLayout;
    warnLayout->setSpacing(6);
    warnLayout->addWidget(warn);
    layout->addItem(warnLayout);
}

EidCertificateAndPinInfo CertificateWidgetInfo::certificateInfo() const
{
    return certAndPinInfo;
}

std::tuple<QString, QString, QString, QString> CertificateWidgetInfo::certData() const
{
    return {certAndPinInfo.certInfo.subject.toHtmlEscaped(),
            certAndPinInfo.certificate.issuerInfo(QSslCertificate::CommonName)
                .join(' ')
                .toHtmlEscaped(),
            certAndPinInfo.certificate.effectiveDate().date().toString(Qt::ISODate),
            certAndPinInfo.certificate.expiryDate().date().toString(Qt::ISODate)};
}

void CertificateWidgetInfo::setCertificateInfo(const EidCertificateAndPinInfo& cardCertPinInfo)
{
    warn->setText(CertificateWidget::tr("Pin locked"));
    certAndPinInfo = cardCertPinInfo;
    const auto& certInfo = cardCertPinInfo.certInfo;
    QString warning;
    auto [subject, issuer, effectiveDate, expiryDate] = certData();
    bool isError =
        certInfo.notEffective || certInfo.isExpired || cardCertPinInfo.pinInfo.pinIsBlocked();
    if (certInfo.notEffective) {
        effectiveDate = displayInRed(effectiveDate);
        warning = displayInRed(CertificateWidget::tr(" (Not effective)"));
    }
    if (certInfo.isExpired) {
        expiryDate = displayInRed(expiryDate);
        warning = displayInRed(CertificateWidget::tr(" (Expired)"));
    }
    info->setText(CertificateWidget::tr("<b>%1</b><br />Issuer: %2<br />Valid: %3 to %4%5")
                      .arg(subject, issuer, effectiveDate, expiryDate, warning));
    info->parentWidget()->setDisabled(isError);
    warn->setVisible(warning.isEmpty() && cardCertPinInfo.pinInfo.pinIsBlocked());
    if (isError) {
        icon->setPixmap(Application::isDarkTheme() ? QStringLiteral(":/images/id-card-err_dark.svg")
                                                   : QStringLiteral(":/images/id-card-err.svg"));
    } else {
        icon->setPixmap(Application::isDarkTheme() ? QStringLiteral(":/images/id-card_dark.svg")
                                                   : QStringLiteral(":/images/id-card.svg"));
    }
}

void CertificateWidgetInfo::languageChange()
{
    setCertificateInfo(certAndPinInfo);
}

CertificateWidget::CertificateWidget(QWidget* parent) : QWidget(parent), CertificateWidgetInfo(this)
{
    info->setFocusPolicy(Qt::TabFocus);
}

void CertificateWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    QStyleOptionButton opt;
    opt.initFrom(this);
    // Applies style sheet styling to the custom widget.
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

CertificateButton::CertificateButton(const EidCertificateAndPinInfo& cardCertPinInfo,
                                     QWidget* parent) :
    QAbstractButton(parent), CertificateWidgetInfo(this)
{
    setCheckable(true);
    setAutoExclusive(true);
    CertificateWidgetInfo::icon->setAttribute(Qt::WA_TransparentForMouseEvents);
    info->setAttribute(Qt::WA_TransparentForMouseEvents);
    setCertificateInfo(cardCertPinInfo);
}

void CertificateButton::setCertificateInfo(const EidCertificateAndPinInfo& cardCertPinInfo)
{
    CertificateWidgetInfo::setCertificateInfo(cardCertPinInfo);
    auto [subject, issuer, effectiveDate, expiryDate] = certData();
    setText(tr("%1 Issuer: %2 Valid: %3 to %4").arg(subject, issuer, effectiveDate, expiryDate));
}

void CertificateButton::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    QStyleOptionButton opt;
    opt.initFrom(this);
    if (isChecked()) {
        opt.state |= QStyle::State_On;
    }
    style()->drawControl(QStyle::CE_PushButton, &opt, &p, this);
}
