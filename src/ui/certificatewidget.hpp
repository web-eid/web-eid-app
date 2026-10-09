// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <QAbstractButton>

#include "certandpininfo.hpp"

class QLabel;

class CertificateWidgetInfo
{
public:
    virtual ~CertificateWidgetInfo() = default;
    EidCertificateAndPinInfo certificateInfo() const;
    virtual void setCertificateInfo(const EidCertificateAndPinInfo& certAndPinInfo);
    void languageChange();

protected:
    explicit CertificateWidgetInfo(QWidget* self);
    Q_DISABLE_COPY_MOVE(CertificateWidgetInfo)

    std::tuple<QString, QString, QString, QString> certData() const;

    QLabel* icon;
    QLabel* info;
    QLabel* warn;
    EidCertificateAndPinInfo certAndPinInfo;
};

class CertificateWidget final : public QWidget, public CertificateWidgetInfo
{
    Q_OBJECT

public:
    explicit CertificateWidget(QWidget* parent);

private:
    void paintEvent(QPaintEvent* event) final;
};

class CertificateButton final : public QAbstractButton, public CertificateWidgetInfo
{
    Q_OBJECT

public:
    CertificateButton(const EidCertificateAndPinInfo& certAndPinInfo, QWidget* parent);

private:
    void setCertificateInfo(const EidCertificateAndPinInfo& certAndPinInfo) final;
    void paintEvent(QPaintEvent* event) final;
};
