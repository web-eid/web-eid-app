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

#include "application.hpp"
#include "controller.hpp"
#include "logging.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#import <Cocoa/Cocoa.h>
#import <SafariServices/SafariServices.h>

#include "shared.hpp"

@implementation NSApplication (MacController)

+ (QVariant)toQVariant:(id)data {
    if (data == nil) {
        return {};
    }
    if ([data isKindOfClass:NSString.class]) {
        return QString::fromNSString(data);
    }
    if ([data isKindOfClass:NSData.class]) {
        return QByteArray::fromNSData(data);
    }
    if ([data isKindOfClass:NSDictionary.class]) {
        return [NSApplication toQVariantMap:data];
    }
    if ([data isKindOfClass:NSArray.class]) {
        return [NSApplication toQVariantList:data];
    }
    return {};
}

+ (QVariantList)toQVariantList:(NSArray *)data {
    QVariantList result;
    if (data == nil) {
        return result;
    }
    for (id value in data) {
        result.append([NSApplication toQVariant:value]);
    }
    return result;
}

+ (QVariantMap)toQVariantMap:(NSDictionary *)data {
    QVariantMap result;
    if (data == nil) {
        return result;
    }
    for (id key in data.allKeys) {
        result[QString::fromNSString(key)] = [NSApplication toQVariant:data[key]];
    }
    return result;
}

+ (id)toID:(const QVariant&)data {
    switch (data.type()) {
        case QVariant::String: return data.toString().toNSString();
        case QVariant::Map: return [NSApplication toNSDictionary:data.toMap()];
        case QVariant::List: return [NSApplication toNSArray:data.toList()];
        default: return nil;
    }
}

+ (NSArray*)toNSArray:(const QVariantList&)data {
    NSMutableArray *result = [[NSMutableArray alloc] init];
    for (const QVariant &item: data) {
        [result addObject:[NSApplication toID:item]];
    }
    return result;
}

+ (NSDictionary*)toNSDictionary:(const QVariantMap&)data {
    NSMutableDictionary *result = [[NSMutableDictionary alloc] init];
    for (QVariantMap::const_iterator i = data.cbegin(); i != data.cend(); ++i) {
        result[i.key().toNSString()] = [NSApplication toID:i.value()];
    }
    return result;
}

- (void)notificationEvent:(NSNotification *)notification {
    NSString *nonce = notification.object;
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:WebEidShared];
    NSDictionary *req = [defaults dictionaryForKey:nonce];
    NSLog(@"web-eid-safari: msg from extension nonce (%@) request: %@", nonce, req);
    if (req == nil) {
        return;
    }
    [defaults removeObjectForKey:nonce];
    [defaults synchronize];

    NSDictionary *resp;
    if([@"status" isEqualToString:req[@"command"]]) {
        resp = [NSApplication toNSDictionary:{{QStringLiteral("version"), qApp->applicationVersion()}}];
    } else {
        const auto argumentJson = QJsonDocument::fromJson(QByteArray::fromNSData(req[@"arguments"]));
        Controller controller(std::make_unique<CommandWithArguments>(commandNameToCommandType(QString::fromNSString(req[@"command"])),
                                                                    argumentJson.object().toVariantMap()));
        controller.run();
        QEventLoop e;
        QObject::connect(&controller, &Controller::quit, &e, &QEventLoop::quit);
        e.exec();
        resp = [NSApplication toNSDictionary:controller.result()];
    }

    NSLog(@"web-eid-safari: msg to extension nonce (%@) request: %@", nonce, resp);
    [defaults setObject:resp forKey:nonce];
    [defaults synchronize];
    [NSDistributedNotificationCenter.defaultCenter postNotificationName:WebEidExtension object:nonce userInfo:nil deliverImmediately:YES];
}

@end


static void checkAppAutostart()
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    LSSharedFileListRef list = LSSharedFileListCreate(kCFAllocatorDefault, kLSSharedFileListSessionLoginItems, nil);
    if (list) {
        bool found = false;
        NSURL *url = NSBundle.mainBundle.bundleURL;
        UInt32 seedValue = 0;
        NSArray *loginItemsArray = CFBridgingRelease(LSSharedFileListCopySnapshot(list, &seedValue));
        for (id item in loginItemsArray) {
            if (NSURL *tmp = CFBridgingRelease(LSSharedFileListItemCopyResolvedURL((__bridge LSSharedFileListItemRef)item, 0, nil))) {
                if ([tmp isEqual:url])
                    found = true;
            }
        }

        if (!found) {
            NSDictionary *props = @{(__bridge id)kLSSharedFileListLoginItemHidden: @(YES)};
            LSSharedFileListItemRef item = LSSharedFileListInsertItemURL(
                list, kLSSharedFileListItemLast, nil, nil, (__bridge CFURLRef)url, (__bridge CFDictionaryRef)props, nil);
            if (item) {
                CFRelease(item);
            }
        }
        CFRelease(list);
    }
#pragma clang diagnostic pop
}

static void checkExtensionState()
{
    [SFSafariExtensionManager getStateOfSafariExtensionWithIdentifier:WebEidExtension completionHandler:^(SFSafariExtensionState *state, NSError *error) {
        NSLog(@"Extension state %@, error %@", @(state ? state.enabled : 0), error);
        if (!state.enabled) {
            [SFSafariApplication showPreferencesForExtensionWithIdentifier:WebEidExtension completionHandler:nil];
        }
    }];
}


int main(int argc, char* argv[])
{
    Q_INIT_RESOURCE(web_eid_resources);
    Q_INIT_RESOURCE(translations);

    Application app(argc, argv, QStringLiteral("web-eid-safari"), QStringLiteral("Web eID Safari"));

    try {
        if (auto args = app.parseArgs()) {
            Controller controller(std::move(args));

            QObject::connect(&controller, &Controller::quit, &app, &QApplication::quit);
            // Pass control to Controller::run() when the event loop starts.
            QTimer::singleShot(0, &controller, &Controller::run);

            return QApplication::exec();
        }

    } catch (const ArgumentError& error) {
        // This error must go directly to cerr to avoid extra info from the logging system.
        std::cerr << error.what() << std::endl;
    } catch (const std::exception& error) {
        qCritical() << error;
    }

    [NSDistributedNotificationCenter.defaultCenter addObserver:NSApp selector:@selector(notificationEvent:) name:WebEidApp object:nil];
    checkAppAutostart();
    checkExtensionState();

    return QApplication::exec();
}
