// Copyright 2017 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_API_NAVIGATION_CONTROLLER_H_
#define BRAVE_BROWSER_API_NAVIGATION_CONTROLLER_H_

#include "content/public/browser/web_contents_observer.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace content {
class NavigationController;
}

namespace mate {
class Arguments;
}

namespace brave {

class NavigationController : public mate::Wrappable<NavigationController>,
                             public content::WebContentsObserver {
 public:
  static mate::Handle<NavigationController> CreateFrom(v8::Isolate* isolate,
                                        content::NavigationController* handle);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void GetActiveEntry(mate::Arguments* args) const;
  void GetVisibleEntry(mate::Arguments* args) const;
  void GetCurrentEntryIndex(mate::Arguments* args) const;
  void GetLastCommittedEntry(mate::Arguments* args) const;
  void GetLastCommittedEntryIndex(mate::Arguments* args) const;
  void CanViewSource(mate::Arguments* args) const;
  void GetEntryCount(mate::Arguments* args) const;
  void GetEntryAtIndex(int index, mate::Arguments* args) const;
  void GetEntryAtOffset(int offset, mate::Arguments* args) const;
  void GetPendingEntry(mate::Arguments* args) const;
  void GetPendingEntryIndex(mate::Arguments* args) const;
  void GetTransientEntry(mate::Arguments* args) const;
  void CanGoBack(mate::Arguments* args) const;
  void CanGoForward(mate::Arguments* args) const;
  void CanGoToOffset(int offset, mate::Arguments* args) const;
  void NeedsReload(mate::Arguments* args) const;
  void IsInitialNavigation(mate::Arguments* args) const;
  void IsInitialBlankNavigation(mate::Arguments* args) const;
  void IsValid(mate::Arguments* args) const;

 protected:
  explicit NavigationController(v8::Isolate* isolate,
                            content::NavigationController* handle);
  ~NavigationController() override;

  bool CheckNavigationController(mate::Arguments* args) const;

  // content::WebContentsObserver implementations:
  void WebContentsDestroyed() override;

 private:
  content::NavigationController* navigation_controller_;

  DISALLOW_COPY_AND_ASSIGN(NavigationController);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_API_NAVIGATION_CONTROLLER_H_
