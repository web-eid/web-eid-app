// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "webeiddialog.hpp"

observer_ptr<WebEidUI> WebEidUI::createAndShowDialog(const CommandType command)
{
    auto* dialog = new WebEidDialog {};
    dialog->showWaitingForCardPage(command);
    dialog->show();
    dialog->activateWindow();
    dialog->raise();

    return dialog;
}

void WebEidUI::showAboutPage()
{
    WebEidDialog::showAboutPage();
}

void WebEidUI::showFatalError()
{
    WebEidDialog::showFatalErrorPage();
}
