// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_CRASH_KEYS_H_
#define BRAVE_COMMON_CRASH_KEYS_H_

#include <stddef.h>

#include <set>
#include <string>
#include <vector>

#include "base/debug/crash_logging.h"
#include "base/macros.h"
#include "build/build_config.h"

namespace base {
class CommandLine;
}

namespace crash_keys {

// Registers all of the potential crash keys that can be sent to the crash
// reporting server. Returns the size of the union of all keys.
size_t RegisterCrashKeys();

// Crash Key Name Constants ////////////////////////////////////////////////////

// The URL of the active tab.
extern const char kActiveURL[];

// GPU information.
extern const char kGPUVendorID[];
extern const char kGPUDeviceID[];
extern const char kGPUDriverVersion[];
extern const char kGPUPixelShaderVersion[];
extern const char kGPUVertexShaderVersion[];
#if defined(OS_MACOSX)
extern const char kGPUGLVersion[];
#elif defined(OS_POSIX)
extern const char kGPUVendor[];
extern const char kGPURenderer[];
#endif

#if defined(OS_WIN)
extern const char kHungAudioThreadDetails[];
#endif

#if defined(OS_MACOSX)
namespace mac {

// Used to report the first Cocoa/Mac NSException and its backtrace.
extern const char kFirstNSException[];
extern const char kFirstNSExceptionTrace[];

// Used to report the last Cocoa/Mac NSException and its backtrace.
extern const char kLastNSException[];
extern const char kLastNSExceptionTrace[];

// Records the current NSException as it is being created, and its backtrace.
extern const char kNSException[];
extern const char kNSExceptionTrace[];

// In the CrApplication, records information about the current event's
// target-action.
extern const char kSendAction[];

}  // namespace mac
#endif

// Numbers of active views.
extern const char kViewCount[];

}  // namespace crash_keys

#endif  // BRAVE_COMMON_CRASH_KEYS_H_
