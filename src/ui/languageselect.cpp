// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "languageselect.hpp"
#include "ui_languageselect.h"

#include "application.hpp"

#include <QFile>
#include <QSettings>
#include <QStyle>

LanguageSelect::LanguageSelect(QWidget* parent) :
    QDialog(parent), ui(std::make_unique<Ui::LanguageSelect>())
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    if (Application::isDarkTheme()) {
        if (QFile f(QStringLiteral(":dark-languageselect.qss"));
            f.open(QFile::ReadOnly | QFile::Text)) {
            style()->unpolish(this);
            setStyleSheet(styleSheet() + QTextStream(&f).readAll());
            style()->polish(this);
        }
    }
    auto current = QSettings().value(QStringLiteral("lang")).toString();
    if (auto* btn = findChild<QToolButton*>(current)) {
        btn->setChecked(true);
    }
    connect(ui->langGroup, qOverload<QAbstractButton*>(&QButtonGroup::buttonClicked), this,
            [this](QAbstractButton* action) {
                QSettings().setValue(QStringLiteral("lang"), action->objectName());
                qApp->loadTranslations();
                ui->retranslateUi(this);
            });
    connect(ui->select, &QPushButton::clicked, this, &LanguageSelect::accept);
    connect(ui->cancel, &QPushButton::clicked, this, [this, current] {
        if (current.isEmpty()) {
            QSettings().remove(QStringLiteral("lang"));
        } else {
            QSettings().setValue(QStringLiteral("lang"), current);
        }
        qApp->loadTranslations();
        reject();
    });
}

LanguageSelect::~LanguageSelect() noexcept = default;
