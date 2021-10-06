/*
 * Copyright (c) 2020-2021 Estonian Information System Authority
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
        NSLog(@"web-eid-safari-extension: starting");
        contexts = [[NSMutableDictionary<NSString *, NSExtensionContext *> alloc] init];
        [NSDistributedNotificationCenter.defaultCenter addObserver:self selector:@selector(notificationEvent:) name:WebEidExtension object:nil];
    }
    return self;
}

- (void)dealloc
{
    NSLog(@"web-eid-safari-extension: stopping");
    [NSDistributedNotificationCenter.defaultCenter removeObserver:self name:WebEidExtension object:nil];
}

- (void)notificationEvent:(NSNotification*)notification
{
    // Received notification from App
    NSString *nonce = notification.object;
    NSDictionary *resp = takeValue(nonce);
    NSLog(@"web-eid-safari-extension: from app nonce (%@) request: %@", nonce, resp);
    if (resp == nil) {
        return;
    }

    for (int i = 0; i < 20 && [NSRunningApplication runningApplicationsWithBundleIdentifier:WebEidApp].count > 0; ++i) {
        [NSThread sleepForTimeInterval:0.5];
        NSLog(@"web-eid-safari-extension: web-eid-safari is still running");
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
        NSLog(@"web-eid-safari-extension: failed to get app url");
        return NO;
    }
    setValue(WebEidStarting, @(true));
    if (![NSWorkspace.sharedWorkspace launchApplication:appURL.path]) {
        NSLog(@"web-eid-safari-extension: failed to start app");
        return NO;
    }
    NSLog(@"web-eid-safari-extension: started app");
    for (int i = 0; i < 20 && [getUserDefaults() boolForKey:WebEidStarting]; ++i) {
        [NSThread sleepForTimeInterval:0.5];
        NSLog(@"web-eid-safari-extension: waiting to be running %@", [getUserDefaults() objectForKey:WebEidStarting]);
    }
    if ([(NSNumber*)takeValue(WebEidStarting) boolValue]) {
        NSLog(@"web-eid-safari-extension: timeout to start app");
        return NO;
    }
    NSLog(@"web-eid-safari-extension: app executed");
    return YES;
}

- (void)beginRequestWithExtensionContext:(NSExtensionContext*)context
{
    id message = [context.inputItems.firstObject userInfo][SFExtensionMessageKey];
    NSLog(@"web-eid-safari-extension: msg from background.js %@", message);

    if ([@"status" isEqualToString:message[@"command"]]) {
        NSString *version = [NSString stringWithFormat:@"%@.%@",
                             NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"],
                             NSBundle.mainBundle.infoDictionary[@"CFBundleVersion"]];
        NSExtensionItem *response = [[NSExtensionItem alloc] init];
        response.userInfo = @{ SFExtensionMessageKey: @{@"version": version} };
        [context completeRequestReturningItems:@[ response ] completionHandler:nil];
        return;
    }

    if (![self execNativeApp]) {
        NSDictionary *resp = @{@"code": @"ERR_WEBEID_NATIVE_FATAL", @"message": @"Failed to start app"};
        NSExtensionItem *response = [[NSExtensionItem alloc] init];
        response.userInfo = @{ SFExtensionMessageKey: resp };
        [context completeRequestReturningItems:@[ response ] completionHandler:nil];
        return;
    }

    NSLog(@"web-eid-safari-extension: sending message to app %@", message);

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
