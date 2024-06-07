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

#pragma once

#include "pcsc-cpp/pcsc-cpp.hpp"

#include <QVariantMap>

class WebEidUI;

namespace electronic_id
{
class ElectronicID;
class SignatureAlgorithm;
} // namespace electronic_id

void requireArgumentsAndOptionalLang(QStringList argNames, const QVariantMap& args,
                                     const std::string& argDescriptions);

template <typename T>
T validateAndGetArgument(const QString& argName, const QVariantMap& args, bool allowNull = false);

extern template QString validateAndGetArgument<QString>(const QString& argName,
                                                        const QVariantMap& args, bool allowNull);
extern template QByteArray
validateAndGetArgument<QByteArray>(const QString& argName, const QVariantMap& args, bool allowNull);

pcsc_cpp::byte_vector getPin(const electronic_id::ElectronicID& card, WebEidUI* window);

QVariantMap signatureAlgoToVariantMap(const electronic_id::SignatureAlgorithm signatureAlgo);
