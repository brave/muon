// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/common/crash_keys.h"

#include "base/format_macros.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"

namespace crash_keys {

// #include "components/crash/core/common/crash_keys.h"
// kChunkMaxLength is the platform-specific maximum size that a value in a
// single chunk can be; see base::debug::InitCrashKeys. The maximum lengths
// specified by breakpad include the trailing NULL, so the actual length of the
// chunk is one less.
#if defined(OS_MACOSX)
const size_t kChunkMaxLength = 255;
#else  // OS_MACOSX
const size_t kChunkMaxLength = 63;
#endif  // !OS_MACOSX

// A small crash key, guaranteed to never be split into multiple pieces.
const size_t kSmallSize = 63;

// A medium crash key, which will be chunked on certain platforms but not
// others. Guaranteed to never be more than four chunks.
const size_t kMediumSize = kSmallSize * 4;

// A large crash key, which will be chunked on all platforms. This should be
// used sparingly.
const size_t kLargeSize = kSmallSize * 16;

const char kActiveURL[] = "url-chunk";
const char kFontKeyName[] = "font_key_name";
const char kGPUVendorID[] = "gpu-venid";
const char kGPUDeviceID[] = "gpu-devid";
const char kGPUDriverVersion[] = "gpu-driver";
const char kGPUPixelShaderVersion[] = "gpu-psver";
const char kGPUVertexShaderVersion[] = "gpu-vsver";
#if defined(OS_MACOSX)
const char kGPUGLVersion[] = "gpu-glver";
#elif defined(OS_POSIX)
const char kGPUVendor[] = "gpu-gl-vendor";
const char kGPURenderer[] = "gpu-gl-renderer";
#endif
#if defined(OS_WIN)
const char kHungAudioThreadDetails[] = "hung-audio-thread-details";
#endif
#if defined(OS_MACOSX)
namespace mac {
const char kFirstNSException[] = "firstexception";
const char kFirstNSExceptionTrace[] = "firstexception_bt";
const char kLastNSException[] = "lastexception";
const char kLastNSExceptionTrace[] = "lastexception_bt";
const char kNSException[] = "nsexception";
const char kNSExceptionTrace[] = "nsexception_bt";
const char kSendAction[] = "sendaction";
}  // namespace mac
#endif

const char kViewCount[] = "view-count";

size_t RegisterCrashKeys() {
  base::debug::CrashKey fixed_keys[] = {
    { kActiveURL, kLargeSize },
#if !defined(OS_ANDROID)
    { kGPUVendorID, kSmallSize },
    { kGPUDeviceID, kSmallSize },
#endif
    { kGPUDriverVersion, kSmallSize },
    { kGPUPixelShaderVersion, kSmallSize },
    { kGPUVertexShaderVersion, kSmallSize },
#if defined(OS_MACOSX)
    { kGPUGLVersion, kSmallSize },
#elif defined(OS_POSIX)
    { kGPUVendor, kSmallSize },
    { kGPURenderer, kSmallSize },
#endif
    // content/:
    { "bad_message_reason", kSmallSize },
    { "discardable-memory-allocated", kSmallSize },
    { "discardable-memory-free", kSmallSize },
    { kFontKeyName, kSmallSize},
    { "ppapi_path", kMediumSize },
    { "subresource_url", kLargeSize },
    { "total-discardable-memory-allocated", kSmallSize },
#if defined(OS_MACOSX)
    { mac::kFirstNSException, kMediumSize },
    { mac::kFirstNSExceptionTrace, kMediumSize },
    { mac::kLastNSException, kMediumSize },
    { mac::kLastNSExceptionTrace, kMediumSize },
    { mac::kNSException, kMediumSize },
    { mac::kNSExceptionTrace, kMediumSize },
    { mac::kSendAction, kMediumSize },
    { "channel_error_bt", kMediumSize },
    { "remove_route_bt", kMediumSize },
    { "rwhvm_window", kMediumSize },
#endif
    { kViewCount, kSmallSize },
    // media/:
#if defined(OS_WIN)
    { kHungAudioThreadDetails, kSmallSize },
#endif
  };

  // This dynamic set of keys is used for sets of key value pairs when gathering
  // a collection of data, like command line switches or extension IDs.
  std::vector<base::debug::CrashKey> keys(
      fixed_keys, fixed_keys + arraysize(fixed_keys));

  return base::debug::InitCrashKeys(&keys.at(0), keys.size(), kChunkMaxLength);
}

}  // namespace crash_keys
