// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run/upgrade_util.h"

#include "base/command_line.h"

namespace upgrade_util {

bool RelaunchChromeBrowser(const base::CommandLine& command_line) {
  return false;
}

bool IsUpdatePendingRestart() {
  return false;
}

bool SwapNewChromeExeIfPresent() {
  return false;
}

bool IsRunningOldChrome() {
  return false;
}

bool DoUpgradeTasks(const base::CommandLine& command_line) {
  return false;
}

}  // namespace upgrade_util
