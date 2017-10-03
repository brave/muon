// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/tab_utils.h"

namespace chrome {

bool CanToggleAudioMute(content::WebContents* contents) {
  NOTREACHED();
  return false;
}

TabMutedResult SetTabAudioMuted(content::WebContents* contents,
                                bool mute,
                                TabMutedReason reason,
                                const std::string& extension_id) {
  NOTREACHED();
  return TabMutedResult::FAIL_NOT_ENABLED;
}

bool AreAllTabsMuted(const TabStripModel& tab_strip,
                     const std::vector<int>& indices) {
  NOTREACHED();
  return false;
}

void SetSitesMuted(const TabStripModel& tab_strip,
                   const std::vector<int>& indices,
                   const bool mute) {
  NOTREACHED();
}

bool AreAllSitesMuted(const TabStripModel& tab_strip,
                      const std::vector<int>& indices) {
  NOTREACHED();
  return false;
}

}  // namespace chrome
