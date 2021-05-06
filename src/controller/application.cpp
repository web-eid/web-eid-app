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

#include "application.hpp"
#include "certandpininfo.hpp"
#include "logging.hpp"
#include "retriableerror.hpp"

#include <QDir>
#include <QFontDatabase>
#include <QTranslator>

Application::Application(int& argc, char** argv, const QString& name, const QString& display) :
    QApplication(argc, argv)
{
    setApplicationName(name);
    setApplicationDisplayName(display);
    setApplicationVersion(QStringLiteral(PROJECT_VERSION));
    setOrganizationDomain(QStringLiteral("web-eid.eu"));
    setOrganizationName(QStringLiteral("RIA"));

    QTranslator* translator = new QTranslator(this);
    translator->load(QLocale(), QStringLiteral(":/translations/"));
    QApplication::installTranslator(translator);

    for (const QString& font : QDir(QStringLiteral(":/fonts")).entryList()) {
        QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/%1").arg(font));
    }

    registerMetatypes();
    setupLogging();
}

void Application::registerMetatypes()
{
    qRegisterMetaType<electronic_id::AutoSelectFailed::Reason>();
    qRegisterMetaType<electronic_id::CardInfo::ptr>();
    qRegisterMetaType<std::vector<electronic_id::CardInfo::ptr>>();
    qRegisterMetaType<electronic_id::VerifyPinFailed::Status>();

    qRegisterMetaType<CardCertificateAndPinInfo>();
    qRegisterMetaType<std::vector<CardCertificateAndPinInfo>>();

    qRegisterMetaType<RetriableError>();
}
