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

#pragma once

#include "commands.hpp"
#include "certificateinfo.hpp"

#include <QDialog>

/**
 * The UI interface implemented in the ui and mock-ui projects.
 */
class WebEidUI : public QDialog
{
    Q_OBJECT

public:
    using ptr = std::unique_ptr<WebEidUI>;

    explicit WebEidUI(QWidget* parent = nullptr) : QDialog(parent) {}

    // Factory function that creates and shows the dialog that implements this interface.
    static ptr createAndShowDialog(const CommandType command);

    virtual void switchPage(const CommandType commandType) = 0;

public: // slots
    virtual void
    onReaderMonitorStatusUpdate(const electronic_id::AutoSelectFailed::Reason status) = 0;
    virtual void onCertificateReady(const QString& origin, const CertificateStatus certStatus,
                                    const CertificateInfo& certInfo) = 0;
    virtual void onDocumentHashReady(const QString& docHash) = 0;
    virtual void onSigningCertificateHashMismatch() = 0;
    virtual void onVerifyPinFailed(const electronic_id::VerifyPinFailed::Status status,
                                   const quint8 retriesLeft) = 0;
};
