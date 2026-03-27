// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "controllerchildthread.hpp"

class CommandHandlerRunThread : public ControllerChildThread
{
    Q_OBJECT

public:
    CommandHandlerRunThread(QObject* parent, CommandHandler& handler,
                            std::vector<electronic_id::ElectronicID::ptr> eids) noexcept :
        ControllerChildThread(handler.commandType(), parent), commandHandler(handler),
        eids(std::move(eids))
    {
        // Connect retry signal to retry signal to pass it up from the command handler.
        connect(&commandHandler, &CommandHandler::retry, this, &ControllerChildThread::retry);
    }

private:
    void doRun() override { commandHandler.run(std::move(eids)); }

    CommandHandler& commandHandler;
    std::vector<electronic_id::ElectronicID::ptr> eids;
};
