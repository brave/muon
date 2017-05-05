// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/sync_start_util.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "chrome/browser/profiles/profile.h"

namespace {

void StartSyncProxy(const base::FilePath& profile,
                    syncer::ModelType type) {}

}  // namespace

namespace sync_start_util {

syncer::SyncableService::StartSyncFlare GetFlareForSyncableService(
    const base::FilePath& profile_path) {
  return base::Bind(&StartSyncProxy, profile_path);
}

}  // namespace sync_start_util
