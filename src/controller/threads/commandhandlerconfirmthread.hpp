// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "controllerchildthread.hpp"

class CommandHandlerConfirmThread : public ControllerChildThread
{
    Q_OBJECT

public:
    CommandHandlerConfirmThread(QObject* parent, CommandHandler& handler, WebEidUI* w,
                                const EidCertificateAndPinInfo& certAndPin) :
        ControllerChildThread(handler.commandType(), parent), commandHandler(handler), window(w),
        certAndPinInfo(certAndPin)
    {
    }

signals:
    void completed(const QVariantMap& result);

private:
    void doRun() override
    {
        const auto result = commandHandler.onConfirm(window, certAndPinInfo);
        certAndPinInfo.eid->release();
        emit completed(result);
    }

    CommandHandler& commandHandler;
    WebEidUI* window;
    EidCertificateAndPinInfo certAndPinInfo;
};
