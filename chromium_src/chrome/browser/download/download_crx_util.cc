// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_crx_util.h"

using content::DownloadItem;

namespace download_crx_util {

bool IsExtensionDownload(const DownloadItem& download_item) {
  return false;
}

bool OffStoreInstallAllowedByPrefs(Profile* profile, const DownloadItem& item) {
  return false;
}

}  // namespace crx_util
