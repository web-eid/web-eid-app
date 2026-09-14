// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "commandhandler.hpp"

#include "getcommandhandler-mock.hpp"

// This compilation unit provides a mocked implementation of getCommandHandler()
// which is needed so that QSignalSpy can access the pointer of the command
// handler. See example in
// WebEidTests::certificateReady_validCertificateHasExpectedCertificateSubject()`
// in main.cpp.

using namespace std::string_literals;

const QVariantMap AUTHENTICATE_COMMAND_ARGUMENT = {
    {"challengeNonce", "12345678912345678912345678912345678912345678"},
    {"origin", "https://ria.ee"},
};

const QVariantMap GET_CERTIFICATE_COMMAND_ARGUMENT = {{"origin", "https://dummy-origin"}};

std::unique_ptr<GetCertificate> g_cached_GetCertificate = std::make_unique<GetCertificate>(
    std::pair {CommandType::GET_SIGNING_CERTIFICATE, GET_CERTIFICATE_COMMAND_ARGUMENT});
std::unique_ptr<Authenticate> g_cached_Authenticate = std::make_unique<Authenticate>(
    std::pair {CommandType::AUTHENTICATE, AUTHENTICATE_COMMAND_ARGUMENT});

CommandHandler::ptr getCommandHandler(const CommandWithArguments& cmd)
{
    // The cached global unique_ptr will be nullptr after return.
    if (cmd.first == CommandType::GET_SIGNING_CERTIFICATE) {
        return std::move(g_cached_GetCertificate);
    }
    if (cmd.first == CommandType::AUTHENTICATE) {
        return std::move(g_cached_Authenticate);
    }
    throw std::logic_error("Programming error in "s + __FILE__ + ":"s + __FUNCTION__
                           + "(): unhandled command '"s + std::string(cmd.first) + "'"s);
}
