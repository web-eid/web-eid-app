/*
 * Copyright (c) 2020-2022 Estonian Information System Authority
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

#include <QString>
#include <QUrl>

inline QString fromPunycode(const QUrl& punycodeUrl)
{
    QUrl url {punycodeUrl};
    // QUrl::PrettyDecoded only decodes Punycode on select trusted domains like .com, .org etc.
    // We need to work around that restriction in order to support EU national domains like .fi,
    // .ee, .lv, .lt etc.
    url.setHost(url.host() + QStringLiteral(".com"), QUrl::TolerantMode);
    const auto host = url.host(QUrl::PrettyDecoded);
    return QStringLiteral("%2%3").arg(host.mid(0, host.size() - 4),
                                      url.port() == -1 ? QString()
                                                       : QStringLiteral(":%1").arg(url.port()));
}
