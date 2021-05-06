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

#import <SafariServices/SafariServices.h>

#include "shared.hpp"

#import <AppKit/AppKit.h>

@interface SafariWebExtensionHandler : NSObject <NSExtensionRequestHandling> {
    NSMutableDictionary<NSString *, NSExtensionContext *> *contexts;
}
@end

@implementation SafariWebExtensionHandler

- (id)init {
    if (self = [super init]) {
        contexts = [[NSMutableDictionary<NSString *, NSExtensionContext *> alloc] init];
        [NSDistributedNotificationCenter.defaultCenter addObserver:self selector:@selector(notificationEvent:) name:WebEidExtension object:nil];
    }
    return self;
}

- (void)dealloc {
    [NSDistributedNotificationCenter.defaultCenter removeObserver:self name:WebEidExtension object:nil];
}

- (void)notificationEvent:(NSNotification *)notification {
    // Received notification from App
    NSString *nonce = notification.object;
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:WebEidShared];
    NSDictionary *resp = [defaults dictionaryForKey:nonce];
    NSLog(@"web-eid-safari-extension: from app nonce (%@) request: %@", nonce, resp);
    if (resp == nil) {
        return;
    }
    [defaults removeObjectForKey:nonce];
    [defaults synchronize];

    // Forward to background script
    NSExtensionContext *context = contexts[nonce];
    [contexts removeObjectForKey:nonce];
    NSExtensionItem *response = [[NSExtensionItem alloc] init];
    response.userInfo = @{ SFExtensionMessageKey: resp };
    [context completeRequestReturningItems:@[ response ] completionHandler:nil];
}

- (void)beginRequestWithExtensionContext:(NSExtensionContext *)context
{
    BOOL isRunning = [NSRunningApplication runningApplicationsWithBundleIdentifier:WebEidApp].count > 0;
    NSLog(@"web-eid-safari-extension: web-eid-safari isRunning: %d", isRunning);
    if (!isRunning) {
        if (NSURL *appURL = [NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:WebEidApp]) {
            if ([NSWorkspace.sharedWorkspace launchApplication:appURL.path]) {
                NSLog(@"web-eid-safari-extension: started app");
            } else {
                NSLog(@"web-eid-safari-extension: failed to start app");
            }
        }
        else {
            NSLog(@"web-eid-safari-extension: failed to get app url");
        }
    }

    id message = [context.inputItems.firstObject userInfo][SFExtensionMessageKey];
    NSLog(@"web-eid-safari-extension: msg from background.js %@", message);

    // Save context
    NSString *nonce = [[[NSUUID alloc] init] UUIDString];
    contexts[nonce] = context;

    // Forward message to native application
    NSMutableDictionary *msg = [[NSMutableDictionary alloc] init];
    msg[@"command"] = message[@"command"];
    if (message[@"arguments"]) {
        NSError *error;
        msg[@"arguments"] = [NSJSONSerialization dataWithJSONObject:message[@"arguments"] options:0 error:&error];
    }
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:WebEidShared];
    [defaults setObject:msg forKey:nonce];
    [defaults synchronize];
    [NSDistributedNotificationCenter.defaultCenter postNotificationName:WebEidApp object:nonce userInfo:nil deliverImmediately:YES];
}

@end
