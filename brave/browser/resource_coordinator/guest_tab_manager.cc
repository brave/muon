// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/resource_coordinator/guest_tab_manager.h"

#include "atom/browser/extensions/tab_helper.h"
#include "brave/browser/guest_view/tab_view/tab_view_guest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/browser/frame_host/navigation_controller_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;
using content::WebContents;

namespace content {

RestoreHelper::RestoreHelper(WebContents* contents)
    : WebContentsObserver(contents) {}

void RestoreHelper::ClearNeedsReload() {
  static_cast<WebContentsImpl*>(
      web_contents())->GetController().needs_reload_ = false;
}

void RestoreHelper::RemoveRestoreHelper() {
  web_contents()->RemoveUserData(UserDataKey());
}

}  // namespace content

DEFINE_WEB_CONTENTS_USER_DATA_KEY(content::RestoreHelper);

namespace resource_coordinator {

GuestTabManager::GuestTabManager() : TabManager() {}

WebContents* GuestTabManager::CreateNullContents(
    TabStripModel* model, WebContents* old_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto tab_helper = extensions::TabHelper::FromWebContents(old_contents);
  DCHECK(tab_helper && tab_helper->guest());

  auto embedder = tab_helper->guest()->embedder_web_contents();

  WebContents::CreateParams params(old_contents->GetBrowserContext());
  params.initially_hidden = true;
  auto contents = extensions::TabHelper::CreateTab(embedder, params);
  content::RestoreHelper::CreateForWebContents(contents);
  return contents;
}

void GuestTabManager::DestroyOldContents(WebContents* old_contents) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto tab_helper = extensions::TabHelper::FromWebContents(old_contents);
  DCHECK(tab_helper && tab_helper->guest());
  // Let the guest destroy itself after the detach message has been received
  tab_helper->guest()->SetCanRunInDetachedState(false);
}

void GuestTabManager::TabReplacedAt(TabStripModel* tab_strip_model,
                               content::WebContents* old_contents,
                               content::WebContents* new_contents,
                               int index) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto helper = content::RestoreHelper::FromWebContents(new_contents);
  // prevent the navigation controller from trying to autoload on
  // controller->SetActive(true)
  if (helper)
    helper->ClearNeedsReload();
}

void GuestTabManager::ActiveTabChanged(content::WebContents* old_contents,
                                  content::WebContents* new_contents,
                                  int index,
                                  int reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  TabManager::ActiveTabChanged(old_contents, new_contents, index, reason);
  auto helper = content::RestoreHelper::FromWebContents(new_contents);
  if (helper) {
    helper->RemoveRestoreHelper();

    new_contents->WasHidden();
    new_contents->WasShown();

    auto tab_helper = extensions::TabHelper::FromWebContents(new_contents);
    if (!tab_helper->is_placeholder()) {
      // if the helper is set this is a discarded tab so we need to reload
      new_contents->GetController().Reload(content::ReloadType::NORMAL, true);
    }
  }
}

}  // namespace resource_coordinator
