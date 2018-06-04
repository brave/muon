// Copyright (c) 2016 Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_UI_BRAVE_TAB_STRIP_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_TAB_STRIP_MODEL_DELEGATE_H_

#include <vector>

#include "base/macros.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"

class GURL;

namespace brave {

class BraveTabStripModelDelegate : public TabStripModelDelegate {
 public:
  BraveTabStripModelDelegate();
  ~BraveTabStripModelDelegate() override;

 private:
  // Overridden from TabStripModelDelegate:
  void AddTabAt(const GURL& url, int index, bool foreground) override;
  Browser* CreateNewStripWithContents(
      std::vector<NewStripContents> contentses,
      const gfx::Rect& window_bounds,
      bool maximize) override;
  void WillAddWebContents(content::WebContents* contents) override;
  int GetDragActions() const override;
  bool CanDuplicateContentsAt(int index) override;
  void DuplicateContentsAt(int index) override;
  void CreateHistoricalTab(content::WebContents* contents) override;
  bool RunUnloadListenerBeforeClosing(content::WebContents* contents) override;
  bool ShouldRunUnloadListenerBeforeClosing(
      content::WebContents* contents) override;
  bool CanBookmarkAllTabs() const override;
  void BookmarkAllTabs() override;
  RestoreTabType GetRestoreTabType() override;
  void RestoreTab() override;

  void CloseFrame();

  DISALLOW_COPY_AND_ASSIGN(BraveTabStripModelDelegate);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BRAVE_TAB_STRIP_MODEL_DELEGATE_H_
