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

#include "logging.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>

#include <iostream>

namespace
{

const QString LOG_FILENAME = QStringLiteral("web-eid.log");

bool openLogFile(QFile& logFile)
{
    const auto logFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!logFilePath.isEmpty()) {
        QDir logFileDir {logFilePath};
        if (logFileDir.mkpath(logFileDir.absolutePath())) {
            const auto filePath = logFileDir.filePath(LOG_FILENAME);
            logFile.setFileName(filePath);
            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                return true;
            } else {
                std::cerr << "Unable to open logfile '" << filePath.toStdString() << '\''
                          << std::endl;
            }
        } else {
            std::cerr << "Unable to create logifile directory '"
                      << logFileDir.absolutePath().toStdString() << '\'' << std::endl;
        }
    } else {
        std::cerr << "Unable to determine logfile location path" << std::endl;
    }
    return false;
}

inline const char* toString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return "DEBUG";
    case QtInfoMsg:
        return "INFO";
    case QtWarningMsg:
        return "WARNING";
    case QtCriticalMsg:
        return "ERROR";
    case QtFatalMsg:
        return "FATAL";
    }
    return "UNKNOWN";
}

inline QString removeAbsolutePathPrefix(const QString& filePath)
{
    const auto lastSrc = filePath.lastIndexOf(QStringLiteral("src"));
    return filePath.mid(lastSrc);
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    static QMutex mutex;
    QMutexLocker lock(&mutex);

    static QFile logFile;
    static bool logFileIsOpen = openLogFile(logFile);

    std::cerr << toString(type) << ": " << message.toStdString() << std::endl;

    if (logFileIsOpen) {
        logFile.write(QStringLiteral("%1 %2 %3:%4:%5 - %6\n")
                          .arg(QDateTime::currentDateTimeUtc().toString(
                              QStringLiteral("yyyy-MM-ddThh:mm:ss.zzzZ")))
                          .arg(toString(type))
                          .arg(removeAbsolutePathPrefix(context.file))
                          .arg(context.line)
                          .arg(context.function)
                          .arg(message)
                          .toUtf8());
        logFile.flush();
    }
}

} // namespace

void setupLogging()
{
    qInstallMessageHandler(messageHandler);
}
