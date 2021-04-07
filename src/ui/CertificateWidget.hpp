#pragma once

#include <QWidget>

struct CertificateInfo;
class QHBoxLayout;
class QLabel;

class CertificateWidget: public QWidget {
    Q_OBJECT
public:
    CertificateWidget(QWidget *parent = nullptr);
    CertificateWidget(const CertificateInfo &certInfo, QWidget *parent = nullptr);

    void setCertificateInfo(const CertificateInfo &certInfo);

private:
    QHBoxLayout *layout;
    QLabel *icon;
    QLabel *label;
};
