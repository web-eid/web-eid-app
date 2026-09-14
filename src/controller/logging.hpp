// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <QtDebug>

void setupLogging();

inline QDebug operator<<(QDebug out, const std::exception& e)
{
    out << e.what();
    return out;
}
