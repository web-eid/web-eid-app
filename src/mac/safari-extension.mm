/*
 * Copyright (c) 2020-2024 Estonian Information System Authority
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
#import <os/log.h>

static os_log_t logger = os_log_create("eu.web-eid.web-eid-safari.web-eid-safari-extension", "extension");

@interface SafariWebExtensionHandler : NSObject <NSExtensionRequestHandling> {
    NSMutableDictionary<NSString *, NSExtensionContext *> *contexts;
}
@end

@implementation SafariWebExtensionHandler

- (id)init {
    if (self = [super init]) {
        os_log(logger, "starting");
        contexts = [[NSMutableDictionary<NSString *, NSExtensionContext *> alloc] init];
        // Sender is not authenticated; DNCF is system-wide. This is intentional: the notification
        // is a wake-up signal only — payload is read from the App Group NSUserDefaults, which is
        // gated by team-ID entitlement and inaccessible to other processes.
        [NSDistributedNotificationCenter.defaultCenter addObserver:self selector:@selector(notificationEvent:) name:WebEidExtension object:nil];
    }
    return self;
}

- (void)dealloc
{
    os_log(logger, "stopping");
    [NSDistributedNotificationCenter.defaultCenter removeObserver:self name:WebEidExtension object:nil];
}

- (void)notificationEvent:(NSNotification*)notification
{
    // Received notification from App
    NSString *nonce = notification.object;
    NSDictionary *resp = takeValue(nonce);
    os_log(logger, "from app nonce (%{public}@) request: %@", nonce, resp);
    if (resp == nil) {
        return;
    }

    // Wait for the app to fully exit before returning the response to the page.
    // The app is single-shot and calls QCoreApplication::quit() after posting the response.
    // If we return early and the page immediately sends a follow-up request, execNativeApp
    // would race with the still-shutting-down instance: launchApplication may no-op against
    // it and the WebEidStarting handshake in Loop 1 would then time out.
    for (int i = 0; i < 20 && [NSRunningApplication runningApplicationsWithBundleIdentifier:WebEidApp].count > 0; ++i) {
        [NSThread sleepForTimeInterval:0.5];
        os_log(logger, "web-eid-safari is still running");
    }

    // Forward to background script
    NSExtensionContext *context = contexts[nonce];
    [contexts removeObjectForKey:nonce];
    NSExtensionItem *response = [[NSExtensionItem alloc] init];
    response.userInfo = @{ SFExtensionMessageKey: resp };
    [context completeRequestReturningItems:@[ response ] completionHandler:nil];
}

- (BOOL)execNativeApp
{
    NSURL *appURL = [NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:WebEidApp];
    if (appURL == nil) {
        os_log_error(logger, "failed to get app url");
        return NO;
    }
    setValue(WebEidStarting, @(true));
    if (![NSWorkspace.sharedWorkspace launchApplication:appURL.path]) {
        os_log_error(logger, "failed to start app");
        return NO;
    }
    os_log(logger, "started app");
    for (int i = 0; i < 20 && [getUserDefaults() boolForKey:WebEidStarting]; ++i) {
        [NSThread sleepForTimeInterval:0.5];
        os_log(logger, "waiting to be running %{public}@", [getUserDefaults() objectForKey:WebEidStarting]);
    }
    if ([(NSNumber*)takeValue(WebEidStarting) boolValue]) {
        os_log_error(logger, "timeout to start app");
        return NO;
    }
    os_log(logger, "app executed");
    return YES;
}

- (void)beginRequestWithExtensionContext:(NSExtensionContext*)context
{
    id message = [context.inputItems.firstObject userInfo][SFExtensionMessageKey];
    os_log(logger, "msg from background.js %@", message);

    if ([@"status" isEqualToString:message[@"command"]]) {
        NSString *version = [NSString stringWithFormat:@"%@+%@",
                             NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"],
                             NSBundle.mainBundle.infoDictionary[@"CFBundleVersion"]];
        NSExtensionItem *response = [[NSExtensionItem alloc] init];
        response.userInfo = @{ SFExtensionMessageKey: @{@"version": version} };
        [context completeRequestReturningItems:@[ response ] completionHandler:nil];
        return;
    }

    if (![self execNativeApp]) {
        NSDictionary *resp = @{@"error": @{@"code": @"ERR_WEBEID_NATIVE_FATAL", @"message": @"Failed to start app"}};
        NSExtensionItem *response = [[NSExtensionItem alloc] init];
        response.userInfo = @{ SFExtensionMessageKey: resp };
        [context completeRequestReturningItems:@[ response ] completionHandler:nil];
        return;
    }

    os_log(logger, "sending message to app %@", message);

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
    setValue(nonce, msg);
    [NSDistributedNotificationCenter.defaultCenter postNotificationName:WebEidApp object:nonce userInfo:nil deliverImmediately:YES];
}

@end
