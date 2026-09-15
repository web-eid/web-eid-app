// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

#include "commands.hpp"

#include "pcsc-cpp/pcsc-cpp.hpp"

CommandWithArguments readCommandFromStdin();

void writeResponseLength(std::ostream& stream, const uint32_t responseLength);
