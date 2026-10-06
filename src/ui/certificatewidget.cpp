/*
 * Copyright (c) 2021-2024 Estonian Information System Authority
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "certificatewidget.hpp"

#include "application.hpp"

#include <QEvent>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QStyleOption>

// We use two separate widgets, CertificateWidget and CertificateButton, for accessibility, to
// support screen readers.

CertificateWidgetInfo::CertificateWidgetInfo(QWidget* self) :
    icon(new QLabel(self)), info(new QLabel(self)), issuer(new QLabel(self)),
    status(new QLabel(self))
{
    info->setObjectName(QStringLiteral("certificateName"));
    info->setTextFormat(Qt::PlainText);
    info->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    issuer->setObjectName(QStringLiteral("certificateIssuer"));
    issuer->setTextFormat(Qt::PlainText);
    issuer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    status->setObjectName(QStringLiteral("certificateStatus"));
    status->setTextFormat(Qt::PlainText);
    status->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto* layout = new QGridLayout(self);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setHorizontalSpacing(16);
    layout->setVerticalSpacing(2);
    layout->addWidget(icon, 0, 0, 3, 1, Qt::AlignVCenter);
    layout->addWidget(info, 0, 1);
    layout->addWidget(issuer, 1, 1);
    layout->addWidget(status, 2, 1, Qt::AlignLeft);
    layout->setColumnStretch(1, 1);
}

EidCertificateAndPinInfo CertificateWidgetInfo::certificateInfo() const
{
    return certAndPinInfo;
}

std::tuple<QString, QString, QString, QString> CertificateWidgetInfo::certData() const
{
    return {certAndPinInfo.certInfo.subject,
            certAndPinInfo.certificate.issuerInfo(QSslCertificate::CommonName).join(' '),
            certAndPinInfo.certificate.effectiveDate().date().toString(Qt::ISODate),
            certAndPinInfo.certificate.expiryDate().date().toString(Qt::ISODate)};
}

void CertificateWidgetInfo::setCertificateInfo(const EidCertificateAndPinInfo& cardCertPinInfo)
{
    certAndPinInfo = cardCertPinInfo;
    const auto& certInfo = cardCertPinInfo.certInfo;
    QString warning;
    const auto& [subject, issuerName, effectiveDate, expiryDate] = certData();
    Q_UNUSED(effectiveDate)
    bool isError =
        certInfo.notEffective || certInfo.isExpired || cardCertPinInfo.pinInfo.pinIsBlocked();
    if (certInfo.notEffective) {
        warning = CertificateWidget::tr(" (Not effective)");
    }
    if (certInfo.isExpired) {
        warning = CertificateWidget::tr(" (Expired)");
    }
    info->setText(subject);
    issuer->setText(CertificateWidget::tr("Issuer: %1").arg(issuerName));
    status->setText(cardCertPinInfo.pinInfo.pinIsBlocked()
                        ? CertificateWidget::tr("Pin locked")
                        : CertificateWidget::tr("Valid until: %1%2").arg(expiryDate, warning));
    status->setProperty("warning", isError);
    status->style()->unpolish(status);
    status->style()->polish(status);
    info->parentWidget()->setDisabled(isError);
    icon->setPixmap(Application::isDarkTheme() ? QStringLiteral(":/images/id-card_dark.svg")
                                               : QStringLiteral(":/images/id-card.svg"));
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
    issuer->setAttribute(Qt::WA_TransparentForMouseEvents);
    setCertificateInfo(cardCertPinInfo);
}

void CertificateButton::setCertificateInfo(const EidCertificateAndPinInfo& cardCertPinInfo)
{
    CertificateWidgetInfo::setCertificateInfo(cardCertPinInfo);
    auto [subject, issuerName, effectiveDate, expiryDate] = certData();
    setText(
        tr("%1 Issuer: %2 Valid: %3 to %4").arg(subject, issuerName, effectiveDate, expiryDate));
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
