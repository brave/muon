// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_BRAVE_CONTENT_CLIENT_H_
#define BRAVE_COMMON_BRAVE_CONTENT_CLIENT_H_

#include "atom/app/atom_content_client.h"

namespace brave {

class BraveContentClient : public atom::AtomContentClient {
 public:
  void SetActiveURL(const GURL& url) override;
  void SetGpuInfo(const gpu::GPUInfo& gpu_info) override;
};

}  // namespace brave
#endif  // BRAVE_COMMON_BRAVE_CONTENT_CLIENT_H_
