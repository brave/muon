// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/engagement/site_engagement_service.h"

// static
bool SiteEngagementService::IsEnabled() {
  return false;
}

// static
SiteEngagementService* SiteEngagementService::Get(Profile* profile) {
  return nullptr;
}
