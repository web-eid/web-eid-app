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

#pragma once

#include <QAbstractButton>

#include "certandpininfo.hpp"
#include "utils/qdisablecopymove.hpp"

class QLabel;

class CertificateWidgetInfo
{
public:
    virtual ~CertificateWidgetInfo() = default;
    CardCertificateAndPinInfo certificateInfo() const;
    virtual void setCertificateInfo(const CardCertificateAndPinInfo& cardCertPinInfo);
    void languageChange();

protected:
    explicit CertificateWidgetInfo(QWidget* self);
    Q_DISABLE_COPY_MOVE(CertificateWidgetInfo)

    void drawWarnIcon();
    std::tuple<QString, QString, QString, QString> certData() const;

    QLabel* icon;
    QLabel* info;
    QLabel* warnIcon;
    QLabel* warn;
    CardCertificateAndPinInfo certAndPinInfo;
};

class CertificateWidget final : public QWidget, public CertificateWidgetInfo
{
    Q_OBJECT

public:
    explicit CertificateWidget(QWidget* parent);

private:
    bool eventFilter(QObject* object, QEvent* event) final;
    void paintEvent(QPaintEvent* event) final;
};

class CertificateButton final : public QAbstractButton, public CertificateWidgetInfo
{
    Q_OBJECT

public:
    CertificateButton(const CardCertificateAndPinInfo& cardCertPinInfo, QWidget* parent);

private:
    bool eventFilter(QObject* object, QEvent* event) final;
    void setCertificateInfo(const CardCertificateAndPinInfo& cardCertPinInfo) final;
    void paintEvent(QPaintEvent* event) final;
};
