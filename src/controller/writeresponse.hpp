// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <QVariantMap>
#include <string>

void writeResponseToStdOut(bool isInStdinMode, const QVariantMap& result,
                           const std::string& commandType);
