// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run/first_run.h"

namespace first_run {

bool IsChromeFirstRun() {
  return false;
}

base::Time GetFirstRunSentinelCreationTime() {
  return base::Time();
}

}  // namespace first_run
