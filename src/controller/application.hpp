// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "commands.hpp"

#include <QApplication>

#include <stdexcept>

#ifdef Q_OS_MAC
#include <QMenuBar>
#include <memory>
#endif

class ArgumentError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int& argc, char** argv, const QString& name);

    static bool isDarkTheme();
    void loadTranslations(const QString& lang = {});
    static CommandWithArgumentsPtr parseArgs();
    static void registerMetatypes();

    // Methods specific to Safari web extension's containing app,
    // see class SafariApplication in src/mac/main.mm and WebEidDialog::showAboutPage().
    virtual bool isSafariExtensionContainingApp() { return false; }
    virtual void requestSafariExtensionState() {}
#ifdef Q_OS_MAC
    void showAbout();
#endif
    virtual void showSafariSettings() {}

signals:
    void safariExtensionEnabled(bool value);

private:
    QTranslator* translator;
#ifdef Q_OS_MAC
    std::unique_ptr<QMenuBar> menuBar;
#endif
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application*>(QCoreApplication::instance()))
