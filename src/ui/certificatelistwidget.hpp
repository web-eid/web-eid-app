#pragma once

#include <QListWidget>

struct CardCertificateAndPinInfo;
class QLabel;

class CertificateWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit CertificateWidget(QWidget* parent = nullptr);
    explicit CertificateWidget(const CardCertificateAndPinInfo& cardCertPinInfo,
                               QWidget* parent = nullptr);

    void setCertificateInfo(const CardCertificateAndPinInfo& cardCertPinInfo);

private:
    void paintEvent(QPaintEvent* event) final;

    QLabel* icon;
    QLabel* info;
};

class CertificateListWidget : public QListWidget
{
    Q_OBJECT

public:
    using QListWidget::QListWidget;

    void setCertificateInfo(const std::vector<CardCertificateAndPinInfo>& certificateAndPinInfos);
    CardCertificateAndPinInfo selectedCertificate() const;

private:
    void addCertificateListItem(const CardCertificateAndPinInfo& cardCertPinInfo);
    void resizeHeight();
};
