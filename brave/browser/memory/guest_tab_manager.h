// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_MEMORY_GUEST_TAB_MANAGER_H_
#define BRAVE_BROWSER_MEMORY_GUEST_TAB_MANAGER_H_

#include "chrome/browser/memory/tab_manager.h"

namespace content {
class WebContents;
}

namespace memory {

class GuestTabManager : public TabManager {
 public:
  GuestTabManager();

 private:
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                       content::WebContents* old_contents,
                       content::WebContents* new_contents,
                       int index) override;

  content::WebContents* CreateNullContents(
      TabStripModel* model, content::WebContents* old_contents) override;
  void DestroyOldContents(content::WebContents* old_contents) override;

  DISALLOW_COPY_AND_ASSIGN(GuestTabManager);
};

}  // namespace memory

#endif  // BRAVE_BROWSER_MEMORY_GUEST_TAB_MANAGER_H_
