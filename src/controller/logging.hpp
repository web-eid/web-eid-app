// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <QtDebug>

void setupLogging();

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
template <typename... Args>
inline QDebug operator<<(QDebug out, const std::basic_string<char, Args...>& s)
{
    return out << QUtf8StringView(s);
}
#endif

inline QDebug operator<<(QDebug out, const std::exception& e)
{
    out << e.what();
    return out;
}
