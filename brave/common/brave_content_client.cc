// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/common/brave_content_client.h"

#include "base/strings/stringprintf.h"
#include "brave/common/crash_keys.h"
#include "gpu/config/gpu_info.h"
#include "url/gurl.h"

namespace brave {

void BraveContentClient::SetActiveURL(const GURL& url) {
  AtomContentClient::SetActiveURL(url);
  base::debug::SetCrashKeyValue(crash_keys::kActiveURL,
                                url.possibly_invalid_spec());
}

void BraveContentClient::SetGpuInfo(const gpu::GPUInfo& gpu_info) {
  AtomContentClient::SetGpuInfo(gpu_info);
  base::debug::SetCrashKeyValue(crash_keys::kGPUVendorID,
      base::StringPrintf("0x%04x", gpu_info.gpu.vendor_id));
  base::debug::SetCrashKeyValue(crash_keys::kGPUDeviceID,
      base::StringPrintf("0x%04x", gpu_info.gpu.device_id));
  base::debug::SetCrashKeyValue(crash_keys::kGPUDriverVersion,
      gpu_info.driver_version);
  base::debug::SetCrashKeyValue(crash_keys::kGPUPixelShaderVersion,
      gpu_info.pixel_shader_version);
  base::debug::SetCrashKeyValue(crash_keys::kGPUVertexShaderVersion,
      gpu_info.vertex_shader_version);
#if defined(OS_MACOSX)
  base::debug::SetCrashKeyValue(crash_keys::kGPUGLVersion, gpu_info.gl_version);
#elif defined(OS_POSIX)
  base::debug::SetCrashKeyValue(crash_keys::kGPUVendor, gpu_info.gl_vendor);
  base::debug::SetCrashKeyValue(crash_keys::kGPURenderer, gpu_info.gl_renderer);
#endif
}

}  // namespace brave
