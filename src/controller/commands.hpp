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

#include <memory>

#include <QMetaType>
#include <QVariantMap>

class CommandType
{
public:
    enum CommandTypeEnum { INSERT_CARD, GET_CERTIFICATE, AUTHENTICATE, SIGN, NONE = -1 };

    CommandType() = default;
    constexpr CommandType(const CommandTypeEnum _value) : value(_value) {}

    constexpr bool operator==(CommandTypeEnum other) const { return value == other; }
    constexpr bool operator!=(CommandTypeEnum other) const { return value != other; }
    constexpr operator CommandTypeEnum() const { return value; }

    operator std::string() const;

private:
    CommandTypeEnum value = NONE;
};

Q_DECLARE_METATYPE(CommandType);

extern const QString CMDLINE_GET_CERTIFICATE;
extern const QString CMDLINE_AUTHENTICATE;
extern const QString CMDLINE_SIGN;

CommandType commandNameToCommandType(const QString& cmdName);

using CommandWithArguments = std::pair<CommandType, QVariantMap>;
using CommandWithArgumentsPtr = std::unique_ptr<CommandWithArguments>;
