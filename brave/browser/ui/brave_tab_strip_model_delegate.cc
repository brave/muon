// Copyright (c) 2016 Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/ui/brave_tab_strip_model_delegate.h"

namespace brave {

////////////////////////////////////////////////////////////////////////////////
// BraveTabStripModelDelegate, public:

BraveTabStripModelDelegate::BraveTabStripModelDelegate() {
}

BraveTabStripModelDelegate::~BraveTabStripModelDelegate() {
}

////////////////////////////////////////////////////////////////////////////////
// BraveTabStripModelDelegate, TabStripModelDelegate implementation:

void BraveTabStripModelDelegate::AddTabAt(const GURL& url,
                                          int index,
                                          bool foreground) {
  // TODO(bbondy)
}

Browser* BraveTabStripModelDelegate::CreateNewStripWithContents(
    std::vector<NewStripContents> contentses,
    const gfx::Rect& window_bounds,
    bool maximize) {
  // TODO(bbondy)
  // This is only used from chrome/browser/ui/views/tabs/tab_drag_controller.cc
  // which we don't use.
  return nullptr;
}

void BraveTabStripModelDelegate::WillAddWebContents(
    content::WebContents* contents) {
  // TODO(bbondy)
}

int BraveTabStripModelDelegate::GetDragActions() const {
  // TODO(bbondy)
  return TabStripModelDelegate::TAB_MOVE_ACTION;
}

bool BraveTabStripModelDelegate::CanDuplicateContentsAt(int index) {
  // TODO(bbondy)
  return false;
}

void BraveTabStripModelDelegate::DuplicateContentsAt(int index) {
  // TODO(bbondy)
}

void BraveTabStripModelDelegate::CreateHistoricalTab(
    content::WebContents* contents) {
  // TODO(bbondy)
}

bool BraveTabStripModelDelegate::RunUnloadListenerBeforeClosing(
    content::WebContents* contents) {
  // TODO(bbondy)
  return false;
}

bool BraveTabStripModelDelegate::ShouldRunUnloadListenerBeforeClosing(
    content::WebContents* contents) {
  // TODO(bbondy)
  return false;
}

bool BraveTabStripModelDelegate::CanBookmarkAllTabs() const {
  // TODO(bbondy)
  return false;
}

void BraveTabStripModelDelegate::BookmarkAllTabs() {
  // TODO(bbondy)
}

TabStripModelDelegate::RestoreTabType
BraveTabStripModelDelegate::GetRestoreTabType() {
  // TODO(bbondy)
  return TabStripModelDelegate::RESTORE_NONE;
}

void BraveTabStripModelDelegate::RestoreTab() {
  // TODO(bbondy)
}

////////////////////////////////////////////////////////////////////////////////
// BraveTabStripModelDelegate, private:

void BraveTabStripModelDelegate::CloseFrame() {
  // TODO(bbondy)
}

}  // namespace brave
