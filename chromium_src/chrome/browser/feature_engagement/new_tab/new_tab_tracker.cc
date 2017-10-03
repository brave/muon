// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"

namespace {

const int kTwoHoursInMinutes = 120;

}  // namespace

namespace feature_engagement {

NewTabTracker::NewTabTracker(Profile* profile,
                             SessionDurationUpdater* session_duration_updater)
    : FeatureTracker(profile, session_duration_updater) {}

NewTabTracker::~NewTabTracker() = default;

void NewTabTracker::OnNewTabOpened() {}

void NewTabTracker::OnSessionTimeMet() {}

int NewTabTracker::GetSessionTimeRequiredToShowInMinutes() {
  NOTREACHED();
  return kTwoHoursInMinutes;
}

}  // namespace feature_engagement
