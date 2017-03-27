// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_shutdown.h"

namespace browser_shutdown {

namespace {
// Whether the browser is trying to quit (e.g., Quit chosen from menu).
bool g_trying_to_quit = false;
}

void SetTryingToQuit(bool quitting) {
  g_trying_to_quit = quitting;

  if (quitting)
    return;
}

bool IsTryingToQuit() {
  return g_trying_to_quit;
}

ShutdownType GetShutdownType() {
  if (IsTryingToQuit())
    return BROWSER_EXIT;
  else
    return NOT_VALID;
}

}  // namespace browser_shutdown
