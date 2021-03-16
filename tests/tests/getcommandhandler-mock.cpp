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

#include "commandhandler.hpp"

#include "getcommandhandler-mock.hpp"

// This compilation unit provides a mocked implementation of getCommandHandler()
// which is needed so that QSignalSpy can access the pointer of the command
// handler. See example in
// WebEidTests::certificateReady_validCertificateHasExpectedCertificateSubject()`
// in main.cpp.

using namespace std::string_literals;

const QVariantMap AUTHENTICATE_COMMAND_ARGUMENT = {
    {"nonce", "12345678912345678912345678912345678912345678"},
    {"origin", "https://ria.ee"},
    {"origin-cert",
     "MIIBkTCCATegAwIBAgIUHO7Fd2Vd7ie30o8xYHrUOUlLfU0wCgYIKoZIzj0EAwIw"
     "HjELMAkGA1UEBhMCZWUxDzANBgNVBAMMBnJpYS5lZTAeFw0xOTEyMTcyMTA1NTda"
     "Fw0yMDEyMTYyMTA1NTdaMB4xCzAJBgNVBAYTAmVlMQ8wDQYDVQQDDAZyaWEuZWUw"
     "WTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQ+8dFJ3OuJwRW+23o5jwjmH6/dX4l/"
     "hDfCfKys3WLcLp0NtmDRzQqV5N6oOjjjxkp0Ryyk14JSl/CFM45Elj9ao1MwUTAd"
     "BgNVHQ4EFgQU93MZD9u2Um1ciNn3+B/Ed4BXVe8wHwYDVR0jBBgwFoAU93MZD9u2"
     "Um1ciNn3+B/Ed4BXVe8wDwYDVR0TAQH/BAUwAwEB/zAKBggqhkjOPQQDAgNIADBF"
     "AiBmfEe2K+kBdcg2VpDQkUj65gSxuRJubWyKqMp3PbhOzQIhAMEwHl8MXKojadt6"
     "/jYGfoB1Qgp0McItzDpecJ7yxpkc"},
};

const QVariantMap GET_CERTIFICATE_COMMAND_ARGUMENT = {{"type", "auth"},
                                                      {"origin", "https://dummy-origin"}};

std::unique_ptr<GetCertificate> g_cached_GetCertificate = std::make_unique<GetCertificate>(
    std::pair {CommandType::GET_CERTIFICATE, GET_CERTIFICATE_COMMAND_ARGUMENT});
std::unique_ptr<Authenticate> g_cached_Authenticate = std::make_unique<Authenticate>(
    std::pair {CommandType::AUTHENTICATE, AUTHENTICATE_COMMAND_ARGUMENT});

CommandHandler::ptr getCommandHandler(const CommandWithArguments& cmd)
{
    // The cached global unique_ptr will be nullptr after return.
    if (cmd.first == CommandType::GET_CERTIFICATE) {
        return std::move(g_cached_GetCertificate);
    } else if (cmd.first == CommandType::AUTHENTICATE) {
        return std::move(g_cached_Authenticate);
    }
    throw std::logic_error("Programming error in "s + __FILE__ + ":"s + __FUNCTION__
                           + "(): unhandled command '"s + std::string(cmd.first) + "'"s);
}
