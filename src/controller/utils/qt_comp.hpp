// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
namespace Qt::Literals::StringLiterals
{

constexpr inline QLatin1String operator"" _L1(const char* str, size_t size) noexcept
{
    return QLatin1String(str, qsizetype(size));
}

inline QString operator""_s(const char16_t* str, size_t size) noexcept
{
    return QString(QStringPrivate(nullptr, const_cast<char16_t*>(str), qsizetype(size)));
}

} // namespace Qt::Literals::StringLiterals
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
template <typename... Args>
inline QDebug operator<<(QDebug out, const std::basic_string<char, Args...>& s)
{
    return out << QUtf8StringView(s);
}
#endif
