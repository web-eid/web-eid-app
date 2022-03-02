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

#pragma once

#include <QApplication>

#include "commands.hpp"

#include <stdexcept>

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

    void loadTranslations(const QString& lang = {});
    CommandWithArgumentsPtr parseArgs();
    static void registerMetatypes();

    // Methods specific to Safari web extension's containing app,
    // see class SafariApplication in src/mac/main.mm and WebEidDialog::showAboutPage().
    virtual bool isSafariExtensionContainingApp() { return false; }
    virtual void requestSafariExtensionState() {}
    virtual void showSafariSettings() {}

signals:
    void safariExtensionEnabled(bool value);

private:
    QTranslator* translator;
};
