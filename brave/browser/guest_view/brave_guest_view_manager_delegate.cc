// Copyright 2015 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/guest_view/brave_guest_view_manager_delegate.h"

#include "brave/browser/guest_view/tab_view/tab_view_guest.h"
#include "components/guest_view/browser/guest_view_base.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "components/guest_view/common/guest_view_constants.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/guest_view/extension_options/extension_options_guest.h"
#include "extensions/browser/guest_view/extension_view/extension_view_guest.h"
#include "extensions/browser/guest_view/guest_view_events.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"

using guest_view::GuestViewBase;
using guest_view::GuestViewManager;

namespace brave {

BraveGuestViewManagerDelegate::BraveGuestViewManagerDelegate(
    content::BrowserContext* context)
    : extensions::ExtensionsGuestViewManagerDelegate(context),
      context_(context) {
}

BraveGuestViewManagerDelegate::~BraveGuestViewManagerDelegate() {
}

void BraveGuestViewManagerDelegate::RegisterAdditionalGuestViewTypes() {
  GuestViewManager* manager = GuestViewManager::FromBrowserContext(context_);
  manager->RegisterGuestViewType<TabViewGuest>();
  // manager->RegisterGuestViewType<extensions::WebViewGuest>();
  // manager->RegisterGuestViewType<extensions::ExtensionOptionsGuest>();
  // manager->RegisterGuestViewType<extensions::ExtensionViewGuest>();
  // manager->RegisterGuestViewType<extensions::MimeHandlerViewGuest>();
}

}  // namespace brave
