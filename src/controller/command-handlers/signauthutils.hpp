// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

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

void getPin(pcsc_cpp::byte_vector& pin, const electronic_id::ElectronicID& eid, WebEidUI* window);

QVariantMap signatureAlgoToVariantMap(const electronic_id::SignatureAlgorithm signatureAlgo);
