// Copyright 2017 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/api/navigation_handle.h"

#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/object_template_builder.h"

namespace brave {

NavigationHandle::NavigationHandle(v8::Isolate* isolate,
                                    content::NavigationHandle* handle) :
    navigation_handle_(handle) {
  Init(isolate);
  Observe(handle->GetWebContents());
}

NavigationHandle::~NavigationHandle() {
  WebContentsDestroyed();
}

void NavigationHandle::WebContentsDestroyed() {
  Observe(nullptr);
  navigation_handle_ = nullptr;
}

void NavigationHandle::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle == navigation_handle_) {
    WebContentsDestroyed();
  }
}

bool NavigationHandle::CheckNavigationHandle(mate::Arguments* args) const {
  if (!navigation_handle_) {
    args->ThrowError("navigation handle is no longer valid");
    return false;
  }
  return true;
}

void NavigationHandle::GetURL(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetURL());
}

void NavigationHandle::IsInMainFrame(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->IsInMainFrame());
}

void NavigationHandle::IsParentMainFrame(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->IsParentMainFrame());
}

void NavigationHandle::IsRendererInitiated(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->IsRendererInitiated());
}

void NavigationHandle::GetFrameTreeNodeId(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetFrameTreeNodeId());
}

void NavigationHandle::GetParentFrameTreeNodeId(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetParentFrame()->GetFrameTreeNodeId());
}

void NavigationHandle::WasStartedFromContextMenu(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->WasStartedFromContextMenu());
}

void NavigationHandle::GetSearchableFormURL(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetSearchableFormURL());
}

void NavigationHandle::GetSearchableFormEncoding(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetSearchableFormEncoding());
}

void NavigationHandle::GetReloadType(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetReloadType());
}

void NavigationHandle::GetRestoreType(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetRestoreType());
}

void NavigationHandle::IsPost(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->IsPost());
}

void NavigationHandle::GetReferrer(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetReferrer().url);
}

void NavigationHandle::HasUserGesture(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->HasUserGesture());
}

void NavigationHandle::GetPageTransition(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetPageTransition());
}

void NavigationHandle::IsExternalProtocol(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->IsExternalProtocol());
}

void NavigationHandle::GetNetErrorCode(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(static_cast<int>(navigation_handle_->GetNetErrorCode()));
}

void NavigationHandle::IsSameDocument(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->IsSameDocument());
}

void NavigationHandle::WasServerRedirect(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->WasServerRedirect());
}

void NavigationHandle::GetRedirectChain(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetRedirectChain());
}

void NavigationHandle::HasCommitted(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->HasCommitted());
}

void NavigationHandle::IsErrorPage(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->IsErrorPage());
}

void NavigationHandle::HasSubframeNavigationEntryCommitted(
    mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  if (!navigation_handle_->HasCommitted()) {
    args->Return(false);
    return;
  }

  args->Return(navigation_handle_->HasSubframeNavigationEntryCommitted());
}

void NavigationHandle::DidReplaceEntry(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  if (!navigation_handle_->HasCommitted()) {
    args->Return(false);
    return;
  }

  args->Return(navigation_handle_->DidReplaceEntry());
}

void NavigationHandle::ShouldUpdateHistory(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  if (!navigation_handle_->HasCommitted()) {
    args->Return(false);
    return;
  }

  args->Return(navigation_handle_->ShouldUpdateHistory());
}

void NavigationHandle::GetPreviousURL(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  if (!navigation_handle_->HasCommitted()) {
    args->Return(GURL());
    return;
  }

  args->Return(navigation_handle_->GetPreviousURL());
}

void NavigationHandle::GetResponseHeaders(mate::Arguments* args) const {
  if (!CheckNavigationHandle(args))
    return;

  args->Return(navigation_handle_->GetResponseHeaders());
}

void NavigationHandle::IsValid(mate::Arguments* args) const {
  args->Return(navigation_handle_ != nullptr);
}

// static
mate::Handle<NavigationHandle> NavigationHandle::CreateFrom(
    v8::Isolate* isolate,
    content::NavigationHandle* handle) {
  return mate::CreateHandle(isolate, new NavigationHandle(isolate, handle));
}

// static
void NavigationHandle::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "NavigationHandle"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("getURL", &NavigationHandle::GetURL)
      .SetMethod("isInMainFrame", &NavigationHandle::IsInMainFrame)
      .SetMethod("isParentMainFrame", &NavigationHandle::IsParentMainFrame)
      .SetMethod("isRendererInitiated", &NavigationHandle::IsRendererInitiated)
      .SetMethod("getFrameTreeNodeId", &NavigationHandle::GetFrameTreeNodeId)
      .SetMethod("getParentFrameTreeNodeId",
          &NavigationHandle::GetParentFrameTreeNodeId)
      .SetMethod("wasStartedFromContextMenu",
          &NavigationHandle::WasStartedFromContextMenu)
      .SetMethod("getSearchableFormURL",
          &NavigationHandle::GetSearchableFormURL)
      .SetMethod("getSearchableFormEncoding",
          &NavigationHandle::GetSearchableFormEncoding)
      .SetMethod("getReloadType", &NavigationHandle::GetReloadType)
      .SetMethod("getRestoreType", &NavigationHandle::GetRestoreType)
      .SetMethod("isPost", &NavigationHandle::IsPost)
      .SetMethod("getReferrer", &NavigationHandle::GetReferrer)
      .SetMethod("hasUserGesture", &NavigationHandle::HasUserGesture)
      .SetMethod("getPageTransition", &NavigationHandle::GetPageTransition)
      .SetMethod("isExternalProtocol", &NavigationHandle::IsExternalProtocol)
      .SetMethod("getNetErrorCode", &NavigationHandle::GetNetErrorCode)
      .SetMethod("isSameDocument", &NavigationHandle::IsSameDocument)
      .SetMethod("wasServerRedirect", &NavigationHandle::WasServerRedirect)
      .SetMethod("getRedirectChain", &NavigationHandle::GetRedirectChain)
      .SetMethod("hasCommitted", &NavigationHandle::HasCommitted)
      .SetMethod("isErrorPage", &NavigationHandle::IsErrorPage)
      .SetMethod("hasSubframeNavigationEntryCommitted",
          &NavigationHandle::HasSubframeNavigationEntryCommitted)
      .SetMethod("didReplaceEntry", &NavigationHandle::DidReplaceEntry)
      .SetMethod("shouldUpdateHistory", &NavigationHandle::ShouldUpdateHistory)
      .SetMethod("getPreviousURL", &NavigationHandle::GetPreviousURL)
      .SetMethod("getResponseHeaders", &NavigationHandle::GetResponseHeaders)
      .SetMethod("isValid", &NavigationHandle::IsValid);
}

}  // namespace brave
