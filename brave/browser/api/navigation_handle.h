// Copyright 2017 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_API_NAVIGATION_HANDLE_H_
#define BRAVE_BROWSER_API_NAVIGATION_HANDLE_H_

#include "content/public/browser/web_contents_observer.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace content {
class NavigationHandle;
}

namespace mate {
class Arguments;
}

namespace brave {

class NavigationHandle : public mate::Wrappable<NavigationHandle>,
                         public content::WebContentsObserver {
 public:
  static mate::Handle<NavigationHandle> CreateFrom(v8::Isolate* isolate,
                                          content::NavigationHandle* handle);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // navigation handle methods
  void GetURL(mate::Arguments* args) const;
  void IsInMainFrame(mate::Arguments* args) const;
  void IsParentMainFrame(mate::Arguments* args) const;
  void IsRendererInitiated(mate::Arguments* args) const;
  void GetFrameTreeNodeId(mate::Arguments* args) const;
  void GetParentFrameTreeNodeId(mate::Arguments* args) const;
  void WasStartedFromContextMenu(mate::Arguments* args) const;
  void GetSearchableFormURL(mate::Arguments* args) const;
  void GetSearchableFormEncoding(mate::Arguments* args) const;
  void GetReloadType(mate::Arguments* args) const;
  void GetRestoreType(mate::Arguments* args) const;
  void IsPost(mate::Arguments* args) const;
  void GetReferrer(mate::Arguments* args) const;
  void HasUserGesture(mate::Arguments* args) const;
  void GetPageTransition(mate::Arguments* args) const;
  void IsExternalProtocol(mate::Arguments* args) const;
  void GetNetErrorCode(mate::Arguments* args) const;
  void IsSamePage(mate::Arguments* args) const;
  void WasServerRedirect(mate::Arguments* args) const;
  void GetRedirectChain(mate::Arguments* args) const;
  void HasCommitted(mate::Arguments* args) const;
  void IsErrorPage(mate::Arguments* args) const;
  void HasSubframeNavigationEntryCommitted(mate::Arguments* args) const;
  void DidReplaceEntry(mate::Arguments* args) const;
  void ShouldUpdateHistory(mate::Arguments* args) const;
  void GetPreviousURL(mate::Arguments* args) const;
  void GetResponseHeaders(mate::Arguments* args) const;
  void IsValid(mate::Arguments* args) const;

 protected:
  explicit NavigationHandle(v8::Isolate* isolate,
                            content::NavigationHandle* handle);
  ~NavigationHandle() override;

  bool CheckNavigationHandle(mate::Arguments* args) const;

  // content::WebContentsObserver implementations:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void WebContentsDestroyed() override;

 private:
  content::NavigationHandle* navigation_handle_;

  DISALLOW_COPY_AND_ASSIGN(NavigationHandle);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_API_NAVIGATION_HANDLE_H_
