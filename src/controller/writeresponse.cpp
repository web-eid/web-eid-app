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

#include "writeresponse.hpp"
#include "inputoutputmode.hpp"
#include "logging.hpp"

#include <QJsonDocument>
#include <QJsonObject>

namespace
{

QByteArray resultToJson(const QVariantMap& result, const std::string& commandType)
{
    const auto json = QJsonDocument::fromVariant(result);
    if (!json.isObject()) {
        throw std::logic_error("Controller::resultToJson: command " + commandType
                               + " did not return a JSON object");
    }

    return json.toJson(QJsonDocument::Compact);
}

} // namespace

void writeResponseToStdOut(bool isInStdinMode, const QVariantMap& result,
                           const std::string& commandType)
{
    const auto response = resultToJson(result, commandType);

    if (isInStdinMode) {
        writeResponseLength(std::cout, uint32_t(response.size()));
    }

    std::cout << response.toStdString();

    if (isInStdinMode) {
        std::cout.flush(); // Don't write extra newline in stdin/stdout mode.
    } else {
        std::cout << std::endl; // endl flushes the stream.
    }
}
