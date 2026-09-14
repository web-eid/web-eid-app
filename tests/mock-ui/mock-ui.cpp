// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "ui.hpp"
#include "mock-ui.hpp"

WebEidUI* WebEidUI::createAndShowDialog(const CommandType)
{
    return new MockUI;
}

void WebEidUI::showAboutPage() {}
void WebEidUI::showFatalError() {}
