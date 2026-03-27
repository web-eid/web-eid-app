// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

inline void eraseData(QString& data)
{
    // According to docs, constData() never causes a deep copy to occur, so we can abuse it
    // to overwrite the underlying buffer since the underlying data is not really const.
    QChar* chars = const_cast<QChar*>(data.constData());
    for (int i = 0; i < data.length(); ++i) {
        chars[i] = '\0';
    }
}
