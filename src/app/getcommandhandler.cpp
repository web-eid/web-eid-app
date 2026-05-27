// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "commandhandler.hpp"

#include "command-handlers/getcertificate.hpp"
#include "command-handlers/authenticate.hpp"
#include "command-handlers/sign.hpp"

// getCommandHandler() has to be defined in the app project so that a different
// mock implementation can be provided in the tests project.

CommandHandler::ptr getCommandHandler(const CommandWithArguments& cmd)
{
    switch (cmd.first) {
    case CommandType::GET_SIGNING_CERTIFICATE:
        return std::make_unique<GetCertificate>(cmd);
    case CommandType::AUTHENTICATE:
        return std::make_unique<Authenticate>(cmd);
    case CommandType::SIGN:
        return std::make_unique<Sign>(cmd);
    default:
        throw std::invalid_argument("Unknown command '" + std::string(cmd.first)
                                    + "' in getCommandHandler()");
    }
}
