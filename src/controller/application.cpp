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

#include <QCommandLineParser>
#include <QDir>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QProcess>
#include <QSettings>
#include <QTranslator>

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
    setApplicationDisplayName(QStringLiteral("Web eID"));
    setApplicationVersion(QStringLiteral(PROJECT_VERSION));
    setOrganizationDomain(QStringLiteral("web-eid.eu"));
    setOrganizationName(QStringLiteral("RIA"));
    setQuitOnLastWindowClosed(false);

    installTranslator(translator);
    loadTranslations();

    for (const QString& font : QDir(QStringLiteral(":/fonts")).entryList()) {
        QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/%1").arg(font));
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

#ifndef Q_OS_MAC
bool Application::isDarkTheme()
{
#ifdef Q_OS_WIN
    QSettings settings(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    return settings.value("AppsUseLightTheme", 1).toInt() == 0;
#else
    // There is currently no straightforward way to detect dark mode in Linux, but this works for supported OS-s.
    static const bool isDarkTheme = [] {
        QProcess p;
        p.start(QStringLiteral("gsettings"), {"get", "org.gnome.desktop.interface", "color-scheme"});
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
#endif

void Application::loadTranslations(const QString& lang)
{
    static const QStringList SUPPORTED_LANGS {
        QStringLiteral("en"), QStringLiteral("et"), QStringLiteral("fi"), QStringLiteral("hr"),
        QStringLiteral("ru"), QStringLiteral("de"), QStringLiteral("fr"), QStringLiteral("nl"),
        QStringLiteral("cs"), QStringLiteral("sk")};
    QLocale locale;
    QString langSetting = QSettings().value(QStringLiteral("lang"), lang).toString();
    if (SUPPORTED_LANGS.contains(langSetting)) {
        locale = QLocale(langSetting);
    }
    void(translator->load(locale, QStringLiteral(":/translations/")));
}

CommandWithArgumentsPtr Application::parseArgs()
{
    // On Windows Chrome, the native messaging host is also passed a command line argument with a
    // handle to the calling Chrome native window: --parent-window=<decimal handle value>.
    // We don't use it, but need to support it to avoid unknown option errors.
    QCommandLineOption parentWindow(QStringLiteral("parent-window"),
                                    QStringLiteral("Parent window handle (unused)"),
                                    QStringLiteral("parent-window"));

    QCommandLineOption aboutArgument(QStringLiteral("about"),
                                     QStringLiteral("Show Web-eID about window"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral(
        "Application that communicates with the Web eID browser extension via standard input and "
        "output, but also works standalone in command-line mode. Performs PKI cryptographic "
        "operations with eID smart cards for signing and authentication purposes."));

    parser.addHelpOption();
    parser.addOptions({{{"c", "command-line-mode"},
                        "Command-line mode, read commands from command line arguments instead of "
                        "standard input."},
                       aboutArgument,
                       parentWindow});

    static const auto COMMANDS = "'" + CMDLINE_GET_SIGNING_CERTIFICATE + "', '"
        + CMDLINE_AUTHENTICATE + "', '" + CMDLINE_SIGN + "'.";

    parser.addPositionalArgument(
        QStringLiteral("command"),
        QStringLiteral("The command to execute in command-line mode, any of ") + COMMANDS);
    parser.addPositionalArgument(
        QStringLiteral("arguments"),
        QStringLiteral("Arguments to the given command as a JSON-encoded string."));

    parser.process(arguments());

    if (parser.isSet(QStringLiteral("command-line-mode"))) {
        const auto args = parser.positionalArguments();
        if (args.size() != 2) {
            throw ArgumentError("Provide two positional arguments in command-line mode.");
        }
        const auto& command = args.first();
        const auto& arguments = args.at(1);
        if (command == CMDLINE_GET_SIGNING_CERTIFICATE || command == CMDLINE_AUTHENTICATE
            || command == CMDLINE_SIGN) {
            // TODO: add command-specific argument validation
            return std::make_unique<CommandWithArguments>(commandNameToCommandType(command),
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
    qRegisterMetaType<electronic_id::AutoSelectFailed::Reason>();
    qRegisterMetaType<electronic_id::CardInfo::ptr>();
    qRegisterMetaType<std::vector<electronic_id::CardInfo::ptr>>();
    qRegisterMetaType<electronic_id::VerifyPinFailed::Status>();

    qRegisterMetaType<CardCertificateAndPinInfo>();
    qRegisterMetaType<std::vector<CardCertificateAndPinInfo>>();

    qRegisterMetaType<RetriableError>();
}
