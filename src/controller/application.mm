// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#include "application.hpp"

#include <QUrl>

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>

void Application::showAbout()
{
    NSLog(@"web-eid-app: starting app");
    [NSWorkspace.sharedWorkspace openApplicationAtURL:QUrl::fromLocalFile(qApp->applicationFilePath()).toNSURL() configuration:NSWorkspaceOpenConfiguration.configuration completionHandler:^(NSRunningApplication *app, NSError *error) {
        if (app == nil) {
            NSLog(@"web-eid-app: failed to start app: %@", error);
        }
    }];
}
