// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/safe_browsing_service.h"

#include <stddef.h>

#include "chrome/browser/safe_browsing/ui_manager.h"

namespace safe_browsing {

const scoped_refptr<SafeBrowsingUIManager>&
SafeBrowsingService::ui_manager() const {
  static scoped_refptr<SafeBrowsingUIManager> empty;
  return empty;
}

void SafeBrowsingService::DisableQuicOnIOThread() {}

}  // namespace safe_browsing
