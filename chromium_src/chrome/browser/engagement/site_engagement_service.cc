// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/engagement/site_engagement_service.h"

#include "base/memory/ptr_util.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "chrome/browser/engagement/site_engagement_score.h"

// static
bool SiteEngagementService::IsEnabled() {
  return false;
}

// static
SiteEngagementService* SiteEngagementService::Get(Profile* profile) {
  return nullptr;
}

// static
double SiteEngagementService::GetScoreFromSettings(
    HostContentSettingsMap* settings,
    const GURL& origin) {
  auto clock = base::MakeUnique<base::DefaultClock>();
  return SiteEngagementScore(clock.get(), origin, settings)
      .GetTotalScore();
}
