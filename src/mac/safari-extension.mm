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
