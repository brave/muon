// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_api_frame_id_map_helper_impl.h"

#include "base/bind.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"

namespace extensions {

AtomExtensionApiFrameIdMapHelperImpl::AtomExtensionApiFrameIdMapHelperImpl(
    AtomExtensionApiFrameIdMap* owner)
    : owner_(owner) {
  registrar_.Add(this, chrome::NOTIFICATION_TAB_PARENTED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

AtomExtensionApiFrameIdMapHelperImpl::~AtomExtensionApiFrameIdMapHelperImpl() {}

void AtomExtensionApiFrameIdMapHelperImpl::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_TAB_PARENTED, type);
  content::WebContents* web_contents =
      content::Source<content::WebContents>(source).ptr();
  SessionTabHelper* session_tab_helper =
      web_contents ? SessionTabHelper::FromWebContents(web_contents) : nullptr;
  if (!session_tab_helper)
    return;
  int tab_id = session_tab_helper->session_id().id();
  int window_id = session_tab_helper->window_id().id();
  web_contents->ForEachFrame(
      base::Bind(&AtomExtensionApiFrameIdMap::UpdateTabAndWindowId,
                 base::Unretained(owner_), tab_id, window_id));
}

}  // namespace extensions
