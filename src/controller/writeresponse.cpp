// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "writeresponse.hpp"
#include "inputoutputmode.hpp"
#include "logging.hpp"

#include <QJsonDocument>
#include <QJsonObject>

#include <iostream>

namespace
{

QByteArray resultToJson(const QVariantMap& result, std::string_view commandType)
{
    const auto json = QJsonDocument::fromVariant(result);
    if (!json.isObject()) {
        throw std::logic_error("Controller::resultToJson: command " + std::string(commandType)
                               + " did not return a JSON object");
    }

    return json.toJson(QJsonDocument::Compact);
}

} // namespace

void writeResponseToStdOut(bool isInStdinMode, const QVariantMap& result,
                           std::string_view commandType)
{
    const auto response = resultToJson(result, commandType);

    if (isInStdinMode) {
        writeResponseLength(std::cout, uint32_t(response.size()));
    }

    std::cout.write(response.constData(), std::streamsize(response.size()));

    if (isInStdinMode) {
        std::cout.flush(); // Don't write extra newline in stdin/stdout mode.
    } else {
        std::cout << std::endl; // endl flushes the stream.
    }
}
