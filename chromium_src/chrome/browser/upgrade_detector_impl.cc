// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/upgrade_detector.h"

// static
UpgradeDetector* UpgradeDetector::GetInstance() {
  static auto* const instance = new UpgradeDetector();
  return instance;
}
