// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_GUEST_VIEW_TAB_VIEW_TAB_VIEW_GUEST_H_
#define BRAVE_BROWSER_GUEST_VIEW_TAB_VIEW_TAB_VIEW_GUEST_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/macros.h"
#include "components/guest_view/browser/guest_view.h"

namespace atom {
namespace api {
class WebContents;
}
}

namespace brave {

class TabViewGuest : public guest_view::GuestView<TabViewGuest> {
 public:
  static GuestViewBase* Create(content::WebContents* owner_web_contents);

  static const char Type[];

 private:
  explicit TabViewGuest(content::WebContents* owner_web_contents);

  ~TabViewGuest() override;

  void WebContentsCreated(content::WebContents* source_contents,
                          int opener_render_process_id,
                          int opener_render_frame_id,
                          const std::string& frame_name,
                          const GURL& target_url,
                          content::WebContents* new_contents) override;
  content::WebContents* OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) override;


  // GuestViewBase implementation.
  content::WebContents* CreateNewGuestWindow(
      const content::WebContents::CreateParams& create_params) override;
  void CreateWebContents(
    const base::DictionaryValue& create_params,
    const WebContentsCreatedCallback& callback) final;
  bool CanRunInDetachedState() const final;
  void DidAttachToEmbedder() final;
  bool ZoomPropagatesFromEmbedderToGuest() const final;
  const char* GetAPINamespace() const final;
  void GuestSizeChangedDueToAutoSize(const gfx::Size& old_size,
                                     const gfx::Size& new_size) final;
  bool IsAutoSizeSupported() const final;
  void WillAttachToEmbedder() final;
  int GetTaskPrefix() const final;
  void GuestReady() final;
  void WillDestroy() final;
  void DidInitialize(const base::DictionaryValue& create_params) final;

  atom::api::WebContents* api_web_contents_;

  // Stores the window name of the main frame of the guest.
  std::string name_;

  // Stores the src URL of the WebView.
  GURL src_;

  // Tracks the name, and target URL of the new window. Once the first
  // navigation commits, we no longer track this information.
  struct NewWindowInfo {
    GURL url;
    std::string name;
    bool changed;
    NewWindowInfo(const GURL& url, const std::string& name) :
        url(url),
        name(name),
        changed(false) {}
  };

  using PendingWindowMap = std::map<TabViewGuest*, NewWindowInfo>;
  PendingWindowMap pending_new_windows_;

  DISALLOW_COPY_AND_ASSIGN(TabViewGuest);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_GUEST_VIEW_TAB_VIEW_TAB_VIEW_GUEST_H_
