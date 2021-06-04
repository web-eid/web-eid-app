/*
 * Copyright (c) 2021 The Web eID Project
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

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStyleOption>

// We use two separate widgets, CertificateWidget and CertificateButton, for accessibility, to
// support screen readers.

CertificateWidgetInfo::CertificateWidgetInfo(QWidget* self) :
    icon(new QLabel(self)), info(new QLabel(self))
{
    icon->setPixmap(QStringLiteral(":/images/id-card.svg"));
    QHBoxLayout* layout = new QHBoxLayout(self);
    layout->addWidget(icon);
    layout->addWidget(info, 1);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(20);
}

CardCertificateAndPinInfo CertificateWidgetInfo::certificateInfo() const
{
    return certAndPinInfo;
}

void CertificateWidgetInfo::setCertificateInfo(const CardCertificateAndPinInfo& cardCertPinInfo)
{
    certAndPinInfo = cardCertPinInfo;
    const auto certInfo = cardCertPinInfo.certInfo;
    info->setText(
        CertificateWidget::tr("<b>%1</b><br />Issuer: %2<br />Valid: from %3 to %4")
            .arg(certInfo.subject, certInfo.issuer, certInfo.effectiveDate, certInfo.expiryDate));
}

CertificateWidget::CertificateWidget(QWidget* parent) : QWidget(parent), CertificateWidgetInfo(this)
{
    info->setFocusPolicy(Qt::TabFocus);
}

void CertificateWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    QStyleOptionButton opt;
    opt.init(this);
    // Applies style sheet styling to the custom widget.
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

CertificateButton::CertificateButton(const CardCertificateAndPinInfo& cardCertPinInfo,
                                     QWidget* parent) :
    QAbstractButton(parent),
    CertificateWidgetInfo(this)
{
    setCheckable(true);
    setAutoExclusive(true);
    CertificateWidgetInfo::icon->setAttribute(Qt::WA_TransparentForMouseEvents);
    info->setAttribute(Qt::WA_TransparentForMouseEvents);
    setCertificateInfo(cardCertPinInfo);
    const auto certInfo = cardCertPinInfo.certInfo;
    setText(
        tr("%1 Issuer: %2 Valid: from %3 to %4")
            .arg(certInfo.subject, certInfo.issuer, certInfo.effectiveDate, certInfo.expiryDate));
}

void CertificateButton::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    QStyleOptionButton opt;
    opt.init(this);
    if (isChecked()) {
        opt.state |= QStyle::State_On;
    }
    style()->drawControl(QStyle::CE_PushButton, &opt, &p, this);
}
