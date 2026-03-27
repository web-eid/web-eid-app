// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>

#include <QMetaType>
#include <QVariantMap>

class CommandType
{
    Q_GADGET
public:
    enum CommandTypeEnum : quint8 {
        NONE,
        INSERT_CARD,
        GET_SIGNING_CERTIFICATE,
        AUTHENTICATE,
        SIGN,
        QUIT,
        ABOUT,
    };
    Q_ENUM(CommandTypeEnum)

    constexpr CommandType(CommandTypeEnum _value = NONE) noexcept : value(_value) { }
    explicit CommandType(const QString& cmdName);

    constexpr bool operator==(CommandTypeEnum other) const noexcept { return value == other; }
    constexpr operator CommandTypeEnum() const noexcept { return value; }

    operator std::string() const;

private:
    CommandTypeEnum value;
};

extern const QString CMDLINE_GET_SIGNING_CERTIFICATE;
extern const QString CMDLINE_AUTHENTICATE;
extern const QString CMDLINE_SIGN;

using CommandWithArguments = std::pair<CommandType, QVariantMap>;
using CommandWithArgumentsPtr = std::unique_ptr<CommandWithArguments>;
