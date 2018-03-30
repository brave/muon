// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/user_event_service_factory.h"

namespace browser_sync {

// static
syncer::UserEventService* UserEventServiceFactory::GetForProfile(
    Profile* profile) {
  return nullptr;
}

}  // namespace browser_sync
