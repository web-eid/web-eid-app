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

#include "application.hpp"
#include "certandpininfo.hpp"
#include "logging.hpp"
#include "retriableerror.hpp"
#include "utils/qt_comp.hpp"

#include <QCommandLineParser>
#include <QDir>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QProcess>
#include <QSettings>
#include <QStyleHints>
#include <QTranslator>

#include <algorithm>
#include <array>

using namespace Qt::Literals::StringLiterals;

inline CommandWithArguments::second_type parseArgumentJson(const QString& argumentStr)
{
    const auto argumentJson = QJsonDocument::fromJson(argumentStr.toUtf8());

    if (!argumentJson.isObject()) {
        throw ArgumentError("parseArgument: Invalid JSON, not an object");
    }

    return argumentJson.object().toVariantMap();
}

Application::Application(int& argc, char** argv, const QString& name) :
    QApplication(argc, argv), translator(new QTranslator(this))
{
    setApplicationName(name);
    setApplicationDisplayName(u"Web eID"_s);
    setApplicationVersion(QStringLiteral(PROJECT_VERSION));
    setOrganizationDomain(u"web-eid.eu"_s);
    setOrganizationName(u"RIA"_s);
    setQuitOnLastWindowClosed(false);

    installTranslator(translator);
    loadTranslations();

    auto list = QUrl::idnWhitelist();
    list.append({
        u"fi"_s,
        u"ee"_s,
        u"lt"_s,
        u"lv"_s,
    });
    QUrl::setIdnWhitelist(list);

    for (const QString& font : QDir(u":/fonts"_s).entryList()) {
        QFontDatabase::addApplicationFont(u":/fonts/%1"_s.arg(font));
    }

    registerMetatypes();
    setupLogging();

#ifdef Q_OS_MAC
    menuBar = std::make_unique<QMenuBar>();
    QAction* about = menuBar->addMenu(tr("&File"))->addAction(tr("&About"));
    about->setMenuRole(QAction::AboutRole);
    connect(about, &QAction::triggered, this, &Application::showAbout);
#endif
}

bool Application::isDarkTheme()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#elif defined(Q_OS_UNIX)
    // There is currently no straightforward way to detect dark mode in Linux, but this works for
    // supported OS-s.
    static const bool isDarkTheme = [] {
        QProcess p;
        p.start(u"gsettings"_s, {u"get"_s, u"org.gnome.desktop.interface"_s, u"color-scheme"_s});
        if (p.waitForFinished()) {
            return p.readAllStandardOutput().contains("dark");
        }
        int text_hsv_value = palette().color(QPalette::WindowText).value();
        int bg_hsv_value = palette().color(QPalette::Window).value();
        return text_hsv_value > bg_hsv_value;
    }();
    return isDarkTheme;
#endif
}

void Application::loadTranslations(const QString& lang)
{
    static constexpr auto SUPPORTED_LANGS = std::to_array<QStringView>(
        {u"en", u"et", u"fi", u"hr", u"ru", u"de", u"fr", u"nl", u"cs", u"sk"});
    QLocale locale;
    QString langSetting = QSettings().value(u"lang"_s, lang).toString();
    if (std::ranges::find(SUPPORTED_LANGS, langSetting) != SUPPORTED_LANGS.cend()) {
        locale = QLocale(langSetting);
    }
    void(translator->load(locale, u":/translations/"_s));
}

CommandWithArgumentsPtr Application::parseArgs()
{
    // On Windows Chrome, the native messaging host is also passed a command line argument with a
    // handle to the calling Chrome native window: --parent-window=<decimal handle value>.
    // We don't use it, but need to support it to avoid unknown option errors.
    QCommandLineOption parentWindow(u"parent-window"_s, u"Parent window handle (unused)"_s,
                                    u"parent-window"_s);

    QCommandLineOption aboutArgument(u"about"_s, u"Show Web-eID about window"_s);
    QCommandLineOption commandLineMode(
        {u"c"_s, u"command-line-mode"_s},
        u"Command-line mode, read commands from command line arguments instead of "
        "standard input."_s);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        u"Application that communicates with the Web eID browser extension via standard input and "
        "output, but also works standalone in command-line mode. Performs PKI cryptographic "
        "operations with eID smart cards for signing and authentication purposes."_s);

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({commandLineMode, aboutArgument, parentWindow});

    static const auto COMMANDS = u"'%1', '%2', '%3'."_s.arg(CMDLINE_GET_SIGNING_CERTIFICATE,
                                                            CMDLINE_AUTHENTICATE, CMDLINE_SIGN);

    parser.addPositionalArgument(
        u"command"_s, u"The command to execute in command-line mode, any of "_s + COMMANDS,
        u"(%1|%2|%3)"_s.arg(CMDLINE_GET_SIGNING_CERTIFICATE, CMDLINE_AUTHENTICATE, CMDLINE_SIGN));
    parser.addPositionalArgument(u"arguments"_s,
                                 u"Arguments to the given command as a JSON-encoded string."_s);

    parser.process(arguments());

    if (parser.isSet(commandLineMode)) {
        const auto args = parser.positionalArguments();
        if (args.size() != 2) {
            throw ArgumentError("Provide two positional arguments in command-line mode.");
        }
        const auto& command = args.first();
        const auto& arguments = args.at(1);
        if (command == CMDLINE_GET_SIGNING_CERTIFICATE || command == CMDLINE_AUTHENTICATE
            || command == CMDLINE_SIGN) {
            // TODO: add command-specific argument validation
            return std::make_unique<CommandWithArguments>(CommandType(command),
                                                          parseArgumentJson(arguments));
        }
        throw ArgumentError("The command has to be one of " + COMMANDS.toStdString());
    }
    if (parser.isSet(parentWindow)) {
        // https://bugs.chromium.org/p/chromium/issues/detail?id=354597#c2
        qDebug() << "Parent window handle is unused" << parser.value(parentWindow);
    }
    if (parser.isSet(aboutArgument)) {
        return std::make_unique<CommandWithArguments>(CommandType::ABOUT, QVariantMap());
    }
    // In Linux, when the application is launched via xdg-desktop-portal from a browser that runs
    // inside a Snap/Flatpack/other container, it doesn't receive the extension origin command-line
    // argument. Hence, a special case is required to run the application in input-output mode
    // instead of opening the about window in Linux.
#ifndef Q_OS_LINUX
    if (arguments().size() == 1) {
        return std::make_unique<CommandWithArguments>(CommandType::ABOUT, QVariantMap());
    }
#endif
    return nullptr;
}

void Application::registerMetatypes()
{
    qRegisterMetaType<electronic_id::ElectronicID::ptr>();
    qRegisterMetaType<std::vector<electronic_id::ElectronicID::ptr>>();
    qRegisterMetaType<electronic_id::VerifyPinFailed::Status>();

    qRegisterMetaType<EidCertificateAndPinInfo>();
    qRegisterMetaType<std::vector<EidCertificateAndPinInfo>>();

    qRegisterMetaType<RetriableError>();
}
