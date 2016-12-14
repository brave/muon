// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_GUEST_VIEW_BRAVE_GUEST_VIEW_MANAGER_DELEGATE_H_
#define BRAVE_BROWSER_GUEST_VIEW_BRAVE_GUEST_VIEW_MANAGER_DELEGATE_H_

#include "extensions/browser/guest_view/extensions_guest_view_manager_delegate.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace guest_view {
class GuestViewBase;
}

namespace brave {

class BraveGuestViewManagerDelegate
    : public extensions::ExtensionsGuestViewManagerDelegate {
 public:
  explicit BraveGuestViewManagerDelegate(content::BrowserContext* context);
  ~BraveGuestViewManagerDelegate() override;
  void RegisterAdditionalGuestViewTypes() override;
  bool IsGuestAvailableToContext(guest_view::GuestViewBase* guest) override;

 private:
  content::BrowserContext* const context_;

  DISALLOW_COPY_AND_ASSIGN(BraveGuestViewManagerDelegate);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_GUEST_VIEW_BRAVE_GUEST_VIEW_MANAGER_DELEGATE_H_
