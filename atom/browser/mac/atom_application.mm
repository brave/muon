// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/mac/atom_application.h"

#include "atom/browser/browser.h"
#include "base/auto_reset.h"
#include "base/strings/sys_string_conversions.h"
#include "content/public/browser/browser_accessibility_state.h"

#include <objc/objc-exception.h>
#include "base/debug/crash_logging.h"
#include "base/debug/stack_trace.h"
#include "brave/common/crash_keys.h"
#include "base/strings/stringprintf.h"

namespace {

objc_exception_preprocessor g_next_preprocessor = nullptr;

id ExceptionPreprocessor(id exception) {
  static bool seen_first_exception = false;

  const char* const kExceptionKey =
      seen_first_exception ? crash_keys::mac::kLastNSException
                           : crash_keys::mac::kFirstNSException;
  NSString* value = [NSString stringWithFormat:@"%@ reason %@",
      [exception name], [exception reason]];
  base::debug::SetCrashKeyValue(kExceptionKey, [value UTF8String]);

  const char* const kExceptionTraceKey =
      seen_first_exception ? crash_keys::mac::kLastNSExceptionTrace
                           : crash_keys::mac::kFirstNSExceptionTrace;
  // This exception preprocessor runs prior to the one in libobjc, which sets
  // the -[NSException callStackReturnAddresses].
  base::debug::SetCrashKeyToStackTrace(kExceptionTraceKey,
                                       base::debug::StackTrace());

  seen_first_exception = true;

  // Forward to the original version.
  if (g_next_preprocessor)
    return g_next_preprocessor(exception);
  return exception;
}

}  // namespace


@implementation AtomApplication

+ (void)initialize {
  // Turn all deallocated Objective-C objects into zombies, keeping
  // the most recent 10,000 of them on the treadmill.
  // ObjcEvilDoers::ZombieEnable(true, 10000);

  if (!g_next_preprocessor) {
    g_next_preprocessor =
        objc_setExceptionPreprocessor(&ExceptionPreprocessor);
  }
}

- (BOOL)sendAction:(SEL)anAction to:(id)aTarget from:(id)sender {
  // The Dock menu contains an automagic section where you can select
  // amongst open windows.  If a window is closed via JavaScript while
  // the menu is up, the menu item for that window continues to exist.
  // When a window is selected this method is called with the
  // now-freed window as |aTarget|.  Short-circuit the call if
  // |aTarget| is not a valid window.
  if (anAction == @selector(_selectWindow:)) {
    // Not using -[NSArray containsObject:] because |aTarget| may be a
    // freed object.
    BOOL found = NO;
    for (NSWindow* window in [self windows]) {
      if (window == aTarget) {
        found = YES;
        break;
      }
    }
    if (!found) {
      return NO;
    }
  }

  // When a Cocoa control is wired to a freed object, we get crashers
  // in the call to |super| with no useful information in the
  // backtrace.  Attempt to add some useful information.

  // If the action is something generic like -commandDispatch:, then
  // the tag is essential.
  NSInteger tag = 0;
  if ([sender isKindOfClass:[NSControl class]]) {
    tag = [sender tag];
    if (tag == 0 || tag == -1) {
      tag = [sender selectedTag];
    }
  } else if ([sender isKindOfClass:[NSMenuItem class]]) {
    tag = [sender tag];
  }

  NSString* actionString = NSStringFromSelector(anAction);
  std::string value = base::StringPrintf("%s tag %ld sending %s to %p",
      [[sender className] UTF8String],
      static_cast<long>(tag),
      [actionString UTF8String],
      aTarget);
  base::debug::ScopedCrashKey key(crash_keys::mac::kSendAction, value);

  return [super sendAction:anAction to:aTarget from:sender];
}

+ (AtomApplication*)sharedApplication {
  return (AtomApplication*)[super sharedApplication];
}

- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)sendEvent:(NSEvent*)event {
  base::AutoReset<BOOL> scoper(&handlingSendEvent_, YES);
  [super sendEvent:event];
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)setCurrentActivity:(NSString*)type
              withUserInfo:(NSDictionary*)userInfo
            withWebpageURL:(NSURL*)webpageURL {
  currentActivity_ = base::scoped_nsobject<NSUserActivity>(
      [[NSUserActivity alloc] initWithActivityType:type]);
  [currentActivity_ setUserInfo:userInfo];
  [currentActivity_ setWebpageURL:webpageURL];
  [currentActivity_ becomeCurrent];
}

- (NSUserActivity*)getCurrentActivity {
  return currentActivity_.get();
}

- (void)awakeFromNib {
  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler:self
          andSelector:@selector(handleURLEvent:withReplyEvent:)
        forEventClass:kInternetEventClass
           andEventID:kAEGetURL];
}

- (void)handleURLEvent:(NSAppleEventDescriptor*)event
        withReplyEvent:(NSAppleEventDescriptor*)replyEvent {
  NSString* url = [
      [event paramDescriptorForKeyword:keyDirectObject] stringValue];
  atom::Browser::Get()->OpenURL(base::SysNSStringToUTF8(url));
}

- (bool)voiceOverEnabled {
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  [defaults addSuiteNamed:@"com.apple.universalaccess"];
  [defaults synchronize];

  return [defaults boolForKey:@"voiceOverOnOffKey"];
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute {
  // Undocumented attribute that VoiceOver happens to set while running.
  // Chromium uses this too, even though it's not exactly right.
  if ([attribute isEqualToString:@"AXEnhancedUserInterface"]) {
    bool enableAccessibility = ([self voiceOverEnabled] && [value boolValue]);
    [self updateAccessibilityEnabled:enableAccessibility];
  }
  return [super accessibilitySetValue:value forAttribute:attribute];
}

- (void)updateAccessibilityEnabled:(BOOL)enabled {
  auto ax_state = content::BrowserAccessibilityState::GetInstance();

  if (enabled) {
    ax_state->OnScreenReaderDetected();
  } else {
    ax_state->DisableAccessibility();
  }

  atom::Browser::Get()->OnAccessibilitySupportChanged();
}

@end
