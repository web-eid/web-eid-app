/*
 * Copyright (c) Estonian Information System Authority
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

#pragma once

static NSString* WebEidApp = @"eu.web-eid.web-eid-safari";
static NSString* WebEidExtension = @"eu.web-eid.web-eid-safari.web-eid-safari-extension";
static NSString* WebEidShared = @"eu.web-eid.web-eid-safari.shared";
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
