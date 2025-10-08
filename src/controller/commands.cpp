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

#include "commands.hpp"

#include <QMetaEnum>

#include <stdexcept>
#include <map>

const QString CMDLINE_GET_SIGNING_CERTIFICATE = QStringLiteral("get-signing-certificate");
const QString CMDLINE_AUTHENTICATE = QStringLiteral("authenticate");
const QString CMDLINE_SIGN = QStringLiteral("sign");
// A special command for stdin mode for quitting the application after sending the version.
const QString STDINMODE_QUIT = QStringLiteral("quit");

CommandType commandNameToCommandType(const QString& cmdName)
{
    static const std::map<QString, CommandType> SUPPORTED_COMMANDS {
        {CMDLINE_GET_SIGNING_CERTIFICATE, CommandType::GET_SIGNING_CERTIFICATE},
        {CMDLINE_AUTHENTICATE, CommandType::AUTHENTICATE},
        {CMDLINE_SIGN, CommandType::SIGN},
        {STDINMODE_QUIT, CommandType::QUIT},
    };

    try {
        return SUPPORTED_COMMANDS.at(cmdName);
    } catch (const std::out_of_range&) {
        throw std::invalid_argument("Command '" + cmdName.toStdString() + "' is not supported");
    }
}

CommandType::operator std::string() const
{
    return QMetaEnum::fromType<CommandType::CommandTypeEnum>().valueToKey(value);
}
