// Copyright 2017 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/api/navigation_controller.h"

#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/web_contents.h"
#include "native_mate/object_template_builder.h"

using mate::Arguments;

namespace brave {

NavigationController::NavigationController(v8::Isolate* isolate,
                                    content::NavigationController* handle) :
    navigation_controller_(handle) {
  Init(isolate);
  Observe(handle->GetWebContents());
}

NavigationController::~NavigationController() {
  WebContentsDestroyed();
}

void NavigationController::WebContentsDestroyed() {
  Observe(nullptr);
  navigation_controller_ = nullptr;
}

bool NavigationController::CheckNavigationController(
    Arguments* args) const {
  if (!navigation_controller_) {
    args->ThrowError("navigation controller is no longer valid");
    return false;
  }
  return true;
}

void NavigationController::GetActiveEntry(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetActiveEntry());
}

void NavigationController::GetVisibleEntry(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetVisibleEntry());
}

void NavigationController::GetCurrentEntryIndex(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetCurrentEntryIndex());
}

void NavigationController::GetLastCommittedEntry(
    Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetLastCommittedEntry());
}

void NavigationController::GetLastCommittedEntryIndex(
    Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetLastCommittedEntryIndex());
}

void NavigationController::CanViewSource(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->CanViewSource());
}

void NavigationController::GetEntryCount(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetEntryCount());
}

void NavigationController::GetEntryAtIndex(int index, Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetEntryAtIndex(index));
}

void NavigationController::GetEntryAtOffset(int offset, Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetEntryAtOffset(offset));
}

void NavigationController::GetPendingEntry(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetPendingEntry());
}

void NavigationController::GetPendingEntryIndex(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetPendingEntryIndex());
}

void NavigationController::GetTransientEntry(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->GetTransientEntry());
}

void NavigationController::CanGoBack(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->CanGoBack());
}

void NavigationController::CanGoForward(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->CanGoForward());
}

void NavigationController::CanGoToOffset(int offset, Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->CanGoToOffset(offset));
}

void NavigationController::NeedsReload(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->NeedsReload());
}

void NavigationController::IsInitialNavigation(Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->IsInitialNavigation());
}

void NavigationController::IsInitialBlankNavigation(
    Arguments* args) const {
  if (!CheckNavigationController(args))
    return;

  args->Return(navigation_controller_->IsInitialBlankNavigation());
}

void NavigationController::IsValid(mate::Arguments* args) const {
  args->Return(navigation_controller_ != nullptr);
}

// static
mate::Handle<NavigationController> NavigationController::CreateFrom(
    v8::Isolate* isolate,
    content::NavigationController* handle) {
  return mate::CreateHandle(isolate, new NavigationController(isolate, handle));
}

// static
void NavigationController::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "NavigationController"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("getActiveEntry", &NavigationController::GetActiveEntry)
      .SetMethod("getVisibleEntry", &NavigationController::GetVisibleEntry)
      .SetMethod("getCurrentEntryIndex",
          &NavigationController::GetCurrentEntryIndex)
      .SetMethod("getLastCommittedEntry",
          &NavigationController::GetLastCommittedEntry)
      .SetMethod("getLastCommittedEntryIndex",
          &NavigationController::GetLastCommittedEntryIndex)
      .SetMethod("canViewSource", &NavigationController::CanViewSource)
      .SetMethod("getEntryCount", &NavigationController::GetEntryCount)
      .SetMethod("getEntryAtIndex", &NavigationController::GetEntryAtIndex)
      .SetMethod("getEntryAtOffset", &NavigationController::GetEntryAtOffset)
      .SetMethod("getPendingEntry", &NavigationController::GetPendingEntry)
      .SetMethod("getPendingEntryIndex",
          &NavigationController::GetPendingEntryIndex)
      .SetMethod("getTransientEntry", &NavigationController::GetTransientEntry)
      .SetMethod("canGoBack", &NavigationController::CanGoBack)
      .SetMethod("canGoForward", &NavigationController::CanGoForward)
      .SetMethod("canGoToOffset", &NavigationController::CanGoToOffset)
      .SetMethod("needsReload", &NavigationController::NeedsReload)
      .SetMethod("isInitialNavigation",
          &NavigationController::IsInitialNavigation)
      .SetMethod("isInitialBlankNavigation",
          &NavigationController::IsInitialBlankNavigation)
      .SetMethod("isValid", &NavigationController::IsValid);
}

}  // namespace brave
