// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/new_tab/new_tab_tracker.h"

namespace {

const int kDefaultPromoShowTimeInHours = 5;

}  // namespace

namespace feature_engagement {

NewTabTracker::NewTabTracker(Profile* profile,
                             SessionDurationUpdater* session_duration_updater)
    : FeatureTracker(profile,
                     session_duration_updater,
                     nullptr, // kIPHNewTabFeature ENABLE_DESKTOP_IN_PRODUCT_HELP
                     base::TimeDelta::FromHours(kDefaultPromoShowTimeInHours)) {
}

NewTabTracker::~NewTabTracker() = default;

void NewTabTracker::OnNewTabOpened() {}

void NewTabTracker::OnSessionTimeMet() {}

}  // namespace feature_engagement
