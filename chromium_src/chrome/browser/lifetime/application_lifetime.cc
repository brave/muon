// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/lifetime/application_lifetime.h"

namespace browser_shutdown {

void NotifyAppTerminating() {
  // NOOP
}

}  // namespace browser_shutdown

namespace chrome {

void OnAppExiting() {
  // NOOP
}

}  // namespace chrome
