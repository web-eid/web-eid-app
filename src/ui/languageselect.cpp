/*
 * Copyright (c) 2020-2024 Estonian Information System Authority
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
    if (Application::isDarkTheme()) {
        if (QFile f(QStringLiteral(":dark-languageselect.qss")); f.open(QFile::ReadOnly | QFile::Text)) {
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
