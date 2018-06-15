// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/upgrade_detector_impl.h"

#include "base/no_destructor.h"
#include "base/time/default_tick_clock.h"

UpgradeDetectorImpl::UpgradeDetectorImpl(const base::TickClock* tick_clock)
    : UpgradeDetector(tick_clock),
      simulating_outdated_(false),
      is_testing_(false),
      weak_factory_(this) {}

UpgradeDetectorImpl::~UpgradeDetectorImpl() {}

base::TimeDelta UpgradeDetectorImpl::GetHighAnnoyanceLevelDelta() {
  return base::TimeDelta();
}

base::TimeTicks UpgradeDetectorImpl::GetHighAnnoyanceDeadline() {
  return base::TimeTicks();
}

void UpgradeDetectorImpl::OnRelaunchNotificationPeriodPrefChanged() {}

void UpgradeDetectorImpl::OnExperimentChangesDetected(Severity severity) {}

// static
UpgradeDetectorImpl* UpgradeDetectorImpl::GetInstance() {
  static base::NoDestructor<UpgradeDetectorImpl> instance(
      base::DefaultTickClock::GetInstance());
  return instance.get();
}

namespace {
class UpgradeDetectorImpl : public UpgradeDetector {
 private:
  // UpgradeDetector overrides:
  // Not used in Brave.
  base::TimeDelta GetHighAnnoyanceLevelDelta() override {
    return base::TimeDelta();
  }

  base::TimeTicks GetHighAnnoyanceDeadline() override {
    return base::TimeTicks();
  }
};
}

// static
UpgradeDetector* UpgradeDetector::GetInstance() {
  return UpgradeDetectorImpl::GetInstance();
}
