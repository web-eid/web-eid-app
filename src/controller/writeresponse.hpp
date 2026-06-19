// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <QVariantMap>

#include <string_view>

void writeResponseToStdOut(bool isInStdinMode, const QVariantMap& result,
                           std::string_view commandType);
