// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

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

constexpr QStringView CMDLINE_GET_SIGNING_CERTIFICATE {u"get-signing-certificate"};
constexpr QStringView CMDLINE_AUTHENTICATE {u"authenticate"};
constexpr QStringView CMDLINE_SIGN {u"sign"};

using CommandWithArguments = std::pair<CommandType, QVariantMap>;
