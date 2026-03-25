// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "inputoutputmode.hpp"

#include <QJsonDocument>
#include <QJsonObject>

#include <iostream>

#ifdef Q_OS_WIN
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifdef Q_OS_WIN
// On Windows, the default I/O mode is O_TEXT. Set this to O_BINARY
// to avoid unwanted modifications of the input/output streams.
// See http://msdn.microsoft.com/en-us/library/tw4k6df8.aspx
void setIoStreamsToBinaryMode()
{
    auto result = _setmode(_fileno(stdin), _O_BINARY);
    if (result == -1)
        throw std::runtime_error("Unable to set stdin to binary mode");

    result = _setmode(_fileno(stdout), _O_BINARY);
    if (result == -1)
        throw std::runtime_error("Unable to set stdout to binary mode");
}
#endif

using namespace pcsc_cpp;

uint32_t readMessageLength(std::istream& input)
{
    uint32_t messageLength = 0;
    input.read(reinterpret_cast<char*>(&messageLength), sizeof(messageLength));
    return messageLength;
}

void writeResponseLength(std::ostream& stream, const uint32_t responseLength)
{
    stream.write(reinterpret_cast<const char*>(&responseLength), sizeof(responseLength));
}

CommandWithArgumentsPtr readCommandFromStdin()
{

#ifdef Q_OS_WIN
    setIoStreamsToBinaryMode();
#endif

    const auto messageLength = readMessageLength(std::cin);

    if (messageLength < 5) {
        throw std::invalid_argument("readCommandFromStdin: Message length is "
                                    + std::to_string(messageLength) + ", at least 5 required");
    }

    if (messageLength > 8192) {
        throw std::invalid_argument("readCommandFromStdin: Message length "
                                    + std::to_string(messageLength)
                                    + " exceeds maximum allowed length 8192");
    }

    auto message = QByteArray(int(messageLength), '\0');
    std::cin.read(message.data(), messageLength);

    const auto json = QJsonDocument::fromJson(message);

    if (!json.isObject()) {
        throw std::invalid_argument("readCommandFromStdin: Invalid JSON, not an object");
    }

    const auto jsonObject = json.object();
    const auto command = jsonObject["command"];
    const auto arguments = jsonObject["arguments"];

    if (!command.isString() || !arguments.isObject()) {
        throw std::invalid_argument("readCommandFromStdin: Invalid JSON, the main object does not "
                                    "contain a 'command' string and 'arguments' object");
    }

    return std::make_unique<CommandWithArguments>(CommandType(command.toString()),
                                                  arguments.toObject().toVariantMap());
}
