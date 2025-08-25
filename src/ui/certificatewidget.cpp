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
    icon(new QLabel(self)), info(new QLabel(self)), warnIcon(new QLabel(self)),
    warn(new QLabel(CertificateWidget::tr("Pin locked"), self))
{
    if (Application::isDarkTheme()) {
        icon->setPixmap(QStringLiteral(":/images/id-card_dark.svg"));
        warnIcon->setPixmap(QStringLiteral(":/images/fatal_dark.svg"));
    } else {
        icon->setPixmap(QStringLiteral(":/images/id-card.svg"));
        warnIcon->setPixmap(QStringLiteral(":/images/fatal.svg"));
    }
    warnIcon->hide();
    warnIcon->installEventFilter(self);
    warn->setObjectName(QStringLiteral("warn"));
    warn->hide();
    auto* layout = new QHBoxLayout(self);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(10);
    layout->addWidget(icon);
    layout->addWidget(info, 1);
    layout->addWidget(warnIcon);
    auto* warnLayout = new QHBoxLayout;
    warnLayout->setSpacing(6);
    warnLayout->addWidget(warnIcon);
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

void CertificateWidgetInfo::drawWarnIcon()
{
    QPainter p(warnIcon);
    QRect cr = warnIcon->contentsRect();
    cr.adjust(warnIcon->margin(), warnIcon->margin(), -warnIcon->margin(), -warnIcon->margin());
    warnIcon->style()->drawItemPixmap(&p, cr, Qt::AlignCenter, warnIcon->pixmap());
}

void CertificateWidgetInfo::setCertificateInfo(const EidCertificateAndPinInfo& cardCertPinInfo)
{
    warn->setText(CertificateWidget::tr("Pin locked"));
    certAndPinInfo = cardCertPinInfo;
    const auto& certInfo = cardCertPinInfo.certInfo;
    QString warning;
    auto [subject, issuer, effectiveDate, expiryDate] = certData();
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
    info->parentWidget()->setDisabled(certInfo.notEffective || certInfo.isExpired
                                      || cardCertPinInfo.pinInfo.pinIsBlocked);
    if (warning.isEmpty() && cardCertPinInfo.pinInfo.pinIsBlocked) {
        warnIcon->show();
        warn->show();
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

bool CertificateWidget::eventFilter(QObject* object, QEvent* event)
{
    if (qobject_cast<QLabel*>(object) && event->type() == QEvent::Paint) {
        drawWarnIcon();
        return true;
    }
    return QWidget::eventFilter(object, event);
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

bool CertificateButton::eventFilter(QObject* object, QEvent* event)
{
    if (qobject_cast<QLabel*>(object) && event->type() == QEvent::Paint) {
        drawWarnIcon();
        return true;
    }
    return QAbstractButton::eventFilter(object, event);
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
