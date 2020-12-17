/*
 * Copyright (c) 2020 The Web eID Project
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

#include "commandhandler.hpp"
#include "controller.hpp"

#include <QApplication>
#include <QCommandLineParser>

#include <QJsonDocument>
#include <QJsonObject>
#include <QtDebug>

class ArgumentError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

inline CommandWithArguments::second_type parseArgumentJson(const QString& argumentStr)
{
    const auto argumentJson = QJsonDocument::fromJson(argumentStr.toUtf8());

    if (!argumentJson.isObject()) {
        // FIXME: implement argument validation with custom exceptions that are sent back up.
        throw std::invalid_argument("parseArgument: Invalid JSON, not an object");
    }

    return argumentJson.object().toVariantMap();
}

inline CommandWithArgumentsPtr parseArgs(const QApplication& app)
{
    QCommandLineOption parentWindow(QStringLiteral("parent-window"),
                                    QStringLiteral("Parent window handle (unused)"),
                                    QStringLiteral("parent-window"));
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Application that communicates with the Web eID browser extension via standard input and "
        "output, but also works standalone in command-line mode. Performs PKI cryptographic "
        "operations with eID smart cards for signing and authentication purposes.");

    parser.addHelpOption();
    parser.addOptions({{{"c", "command-line-mode"},
                        "Command-line mode, read commands from command line arguments instead of "
                        "standard input."},
                       parentWindow});

    static const auto COMMANDS = "'" + CMDLINE_GET_CERTIFICATE + "', '" + CMDLINE_AUTHENTICATE
        + "', '" + CMDLINE_SIGN + "'.";

    parser.addPositionalArgument("command",
                                 "The command to execute in command-line mode, any of " + COMMANDS);
    parser.addPositionalArgument("arguments",
                                 "Arguments to the given command as a JSON-encoded string.");

    parser.process(app);

    if (parser.isSet("command-line-mode")) {
        const auto args = parser.positionalArguments();
        if (args.size() != 2) {
            throw ArgumentError("Provide two positional arguments in command-line mode.");
        }
        const auto command = args.first();
        const auto arguments = args.at(1);
        if (command == CMDLINE_GET_CERTIFICATE || command == CMDLINE_AUTHENTICATE
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

    return nullptr;
}
