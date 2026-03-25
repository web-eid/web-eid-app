// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "commands.hpp"

#include <QMetaEnum>

#include <stdexcept>
#include <map>

const QString CMDLINE_GET_SIGNING_CERTIFICATE = QStringLiteral("get-signing-certificate");
const QString CMDLINE_AUTHENTICATE = QStringLiteral("authenticate");
const QString CMDLINE_SIGN = QStringLiteral("sign");
// A special command for stdin mode for quitting the application after sending the version.
const QString STDINMODE_QUIT = QStringLiteral("quit");

CommandType::CommandType(const QString& cmdName)
{
    static const std::map<QString, CommandType> SUPPORTED_COMMANDS {
        {CMDLINE_GET_SIGNING_CERTIFICATE, CommandType::GET_SIGNING_CERTIFICATE},
        {CMDLINE_AUTHENTICATE, CommandType::AUTHENTICATE},
        {CMDLINE_SIGN, CommandType::SIGN},
        {STDINMODE_QUIT, CommandType::QUIT},
    };

    try {
        value = SUPPORTED_COMMANDS.at(cmdName);
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("Command '" + cmdName.toStdString() + "' is not supported");
    }
}

CommandType::operator std::string() const
{
    return QMetaEnum::fromType<CommandType::CommandTypeEnum>().valueToKey(value);
}
