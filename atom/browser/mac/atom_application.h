// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_MAC_ATOM_APPLICATION_H_
#define ATOM_BROWSER_MAC_ATOM_APPLICATION_H_

#ifdef __OBJC__

#import "base/mac/scoped_sending_event.h"
#import "base/mac/scoped_nsobject.h"

@interface AtomApplication : NSApplication<CrAppProtocol,
                                           CrAppControlProtocol> {
 @private
  BOOL handlingSendEvent_;
  base::scoped_nsobject<NSUserActivity> currentActivity_;
}

+ (AtomApplication*)sharedApplication;

// CrAppProtocol:
- (BOOL)isHandlingSendEvent;

// CrAppControlProtocol:
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

- (NSUserActivity*)getCurrentActivity;
- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL;

@end

#endif  // __OBJC__

namespace atom_application_mac {

void RegisterBrowserCrApp();

}  // namespace atom_application_mac

#endif  // ATOM_BROWSER_MAC_ATOM_APPLICATION_H_
