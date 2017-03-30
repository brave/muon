// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/memory/guest_tab_manager.h"

#include "atom/browser/extensions/tab_helper.h"
#include "brave/browser/guest_view/tab_view/tab_view_guest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"

using content::WebContents;

namespace memory {

GuestTabManager::GuestTabManager() : TabManager() {}

WebContents* GuestTabManager::CreateNullContents(
    TabStripModel* model, WebContents* old_contents) {
  auto tab_helper = extensions::TabHelper::FromWebContents(old_contents);
  DCHECK(tab_helper && tab_helper->guest());

  auto embedder = tab_helper->guest()->embedder_web_contents();

  return extensions::TabHelper::CreateTab(embedder,
      WebContents::CreateParams(old_contents->GetBrowserContext()));
}

void GuestTabManager::DestroyOldContents(WebContents* old_contents) {
  auto tab_helper = extensions::TabHelper::FromWebContents(old_contents);
  DCHECK(tab_helper && tab_helper->guest());
  // Let the guest destroy itself after the detach message has been received
  tab_helper->guest()->SetCanRunInDetachedState(false);
}

void GuestTabManager::ActiveTabChanged(content::WebContents* old_contents,
                                  content::WebContents* new_contents,
                                  int index,
                                  int reason) {
  bool discarded = IsTabDiscarded(new_contents);
  TabManager::ActiveTabChanged(old_contents, new_contents, index, reason);
  if (discarded) {
    if (reason == TabStripModelObserver::CHANGE_REASON_USER_GESTURE)
      new_contents->UserGestureDone();

    new_contents->GetController().Reload(content::ReloadType::NORMAL, true);
  }
}

}  // namespace
