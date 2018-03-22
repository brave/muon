// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/upgrade_detector.h"

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
  static auto* const instance = new UpgradeDetectorImpl();
  return instance;
}
