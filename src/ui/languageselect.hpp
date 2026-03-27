// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <QDialog>

#include <memory>

namespace Ui
{
class LanguageSelect;
}

class LanguageSelect : public QDialog
{
    Q_OBJECT
public:
    explicit LanguageSelect(QWidget* parent = nullptr);
    ~LanguageSelect() noexcept;

private:
    std::unique_ptr<Ui::LanguageSelect> ui;
};
