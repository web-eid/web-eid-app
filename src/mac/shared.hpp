// SPDX-FileCopyrightText: Estonian Information System Authority
// SPDX-License-Identifier: MIT

#pragma once

static NSString* WebEidApp = @"eu.web-eid.web-eid-safari";
static NSString* WebEidExtension = @"eu.web-eid.web-eid-safari.web-eid-safari-extension";
static NSString* WebEidShared = @"ET847QJV9F.eu.web-eid.web-eid-safari.shared";
static NSString* WebEidStarting = @"eu.web-eid.web-eid-safari.starting";

#if __MAC_OS_X_VERSION_MIN_REQUIRED < 110000
NSString* const SFExtensionMessageKey = @"message";
#endif

static NSUserDefaults* getUserDefaults()
{
    return [[NSUserDefaults alloc] initWithSuiteName:WebEidShared];
}

static void setValue(NSString* key, id value)
{
    NSUserDefaults* defaults = getUserDefaults();
    [defaults setObject:value forKey:key];
    [defaults synchronize];
}

static id takeValue(NSString* key)
{
    NSUserDefaults* defaults = getUserDefaults();
    NSDictionary* value = [defaults objectForKey:key];
    if (value == nil) {
        return value;
    }
    [defaults removeObjectForKey:key];
    [defaults synchronize];
    return value;
}
