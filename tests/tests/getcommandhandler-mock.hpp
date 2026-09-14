// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include "command-handlers/getcertificate.hpp"
#include "command-handlers/authenticate.hpp"

extern std::unique_ptr<GetCertificate> g_cached_GetCertificate;
extern std::unique_ptr<Authenticate> g_cached_Authenticate;
extern const QVariantMap AUTHENTICATE_COMMAND_ARGUMENT;
extern const QVariantMap GET_CERTIFICATE_COMMAND_ARGUMENT;
