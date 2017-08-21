// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/lifetime/browser_close_manager.h"

#include "base/callback.h"

BrowserCloseManager::BrowserCloseManager() {}

BrowserCloseManager::~BrowserCloseManager() {}

void BrowserCloseManager::StartClosingBrowsers() {}

void BrowserCloseManager::ConfirmCloseWithPendingDownloads(
    int download_count,
    const base::Callback<void(bool)>& callback) {
  callback.Run(true);
}
