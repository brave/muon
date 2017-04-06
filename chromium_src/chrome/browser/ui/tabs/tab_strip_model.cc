// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "chrome/browser/ui/tabs/tab_strip_model.h"

#include "atom/browser/extensions/tab_helper.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model_order_controller.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ui/base/models/list_selection_model.h"

using content::WebContents;
using extensions::TabHelper;

///////////////////////////////////////////////////////////////////////////////
// WebContentsData

// An object to hold a reference to a WebContents that is in a tabstrip, as
// well as other various properties it has.
class TabStripModel::WebContentsData : public content::WebContentsObserver {
 public:
  WebContentsData(WebContents* a_contents);

  // Changes the WebContents that this WebContentsData tracks.
  void SetWebContents(WebContents* contents);

 private:
  DISALLOW_COPY_AND_ASSIGN(WebContentsData);
};

TabStripModel::WebContentsData::WebContentsData(WebContents* contents)
    : content::WebContentsObserver(contents) {}

void TabStripModel::WebContentsData::SetWebContents(WebContents* contents) {
  Observe(contents);
}

///////////////////////////////////////////////////////////////////////////////
// TabStripModel, public:
TabStripModel::TabStripModel(TabStripModelDelegate* delegate, Profile* profile)
    : delegate_(delegate),
      profile_(profile),
      closing_all_(false),
      in_notify_(false),
      weak_factory_(this) {
  DCHECK(!delegate_);
}

TabStripModel::~TabStripModel() {
  contents_data_.clear();
}

void TabStripModel::AddObserver(TabStripModelObserver* observer) {
  observers_.AddObserver(observer);
}

void TabStripModel::RemoveObserver(TabStripModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool TabStripModel::ContainsIndex(int index) const {
  return index >= 0 && index < count();
}

void TabStripModel::AppendWebContents(WebContents* contents,
                                      bool foreground) {
  // delegate_->WillAddWebContents(contents);
  std::unique_ptr<WebContentsData> data =
      base::MakeUnique<WebContentsData>(contents);

  int index = contents_data_.size();
  contents_data_.push_back(std::move(data));
  for (auto& observer : observers_)
    observer.TabInsertedAt(this, contents, index, foreground);
}

WebContents* TabStripModel::ReplaceWebContentsAt(int index,
                                                 WebContents* new_contents) {
  // delegate_->WillAddWebContents(new_contents);

  DCHECK(ContainsIndex(index));
  WebContents* old_contents = GetWebContentsAt(index);

  for (size_t i = 0; i < contents_data_.size(); ++i) {
    if (contents_data_[i]->web_contents() == old_contents) {
      contents_data_[i]->SetWebContents(new_contents);
      break;
    }
  }

  for (auto& observer : observers_)
    observer.TabReplacedAt(this, old_contents, new_contents, index);

  // When the active WebContents is replaced send out a selection notification
  // too. We do this as nearly all observers need to treat a replacement of the
  // selected contents as the selection changing.
  // TODO(bridiver) - listen for this event and actually swap them
  if (active_index() == index) {
    for (auto& observer : observers_)
      observer.ActiveTabChanged(old_contents, new_contents, active_index(),
                                TabStripModelObserver::CHANGE_REASON_REPLACED);
  }
  return old_contents;
}

WebContents* TabStripModel::DetachWebContentsAt(int index) {
  CHECK(!in_notify_);
  if (contents_data_.empty())
    return nullptr;
  DCHECK(ContainsIndex(index));

  WebContents* removed_contents = GetWebContentsAt(index);

  for (size_t i = 0; i < contents_data_.size(); ++i) {
    if (contents_data_[i]->web_contents() == removed_contents) {
      contents_data_.erase(contents_data_.begin() + i);
      break;
    }
  }

  for (auto& observer : observers_)
    observer.TabDetachedAt(removed_contents, index);

  if (empty()) {
    selection_model_.Clear();
    // TabDetachedAt() might unregister observers, so send |TabStripEmpty()| in
    // a second pass.
    for (auto& observer : observers_)
      observer.TabStripEmpty();
  }
  return removed_contents;
}

void TabStripModel::ActivateTabAt(int index, bool user_gesture) {
  DCHECK(ContainsIndex(index));
  ui::ListSelectionModel new_model;
  new_model.Copy(selection_model_);
  new_model.SetSelectedIndex(index);
  SetSelection(new_model, user_gesture ? NOTIFY_USER_GESTURE : NOTIFY_DEFAULT);
}

WebContents* TabStripModel::GetActiveWebContents() const {
  return GetWebContentsAt(active_index());
}

WebContents* TabStripModel::GetWebContentsAt(int index) const {
  if (index != kNoTab && index < contents_data_.size())
    return contents_data_[index]->web_contents();
  return nullptr;
}

int TabStripModel::GetIndexOfWebContents(const WebContents* contents) const {
  for (size_t i = 0; i < contents_data_.size(); ++i) {
    if (contents_data_[i]->web_contents() == contents)
      return i;
  }
  return kNoTab;
}

void TabStripModel::SetTabPinned(int index, bool pinned) {
  DCHECK(ContainsIndex(index));

  auto tab_helper = TabHelper::FromWebContents(GetWebContentsAt(index));
  DCHECK(tab_helper);

  if (pinned != tab_helper->is_pinned())
    tab_helper->SetPinned(pinned);

  for (auto& observer : observers_)
    observer.TabPinnedStateChanged(this, tab_helper->web_contents(),
                                   index);
}

bool TabStripModel::IsTabPinned(int index) const {
  if (!ContainsIndex(index))
    return false;

  auto tab_helper = TabHelper::FromWebContents(GetWebContentsAt(index));
  DCHECK(tab_helper);

  return tab_helper->is_pinned();
}

bool TabStripModel::IsTabSelected(int index) const {
  if (!ContainsIndex(index))
    return false;

  return selection_model_.IsSelected(index);
}

void TabStripModel::NotifyIfTabDeactivated(WebContents* contents) {
  if (contents) {
    for (auto& observer : observers_)
      observer.TabDeactivated(contents);
  }
}

void TabStripModel::NotifyIfActiveTabChanged(WebContents* old_contents,
                                             NotifyTypes notify_types) {
  WebContents* new_contents = GetWebContentsAt(active_index());
  if (old_contents != new_contents) {
    int reason = notify_types == NOTIFY_USER_GESTURE
                 ? TabStripModelObserver::CHANGE_REASON_USER_GESTURE
                 : TabStripModelObserver::CHANGE_REASON_NONE;
    CHECK(!in_notify_);
    in_notify_ = true;
    for (auto& observer : observers_)
      observer.ActiveTabChanged(old_contents, new_contents, active_index(),
                                reason);
    in_notify_ = false;
  }
}

void TabStripModel::NotifyIfActiveOrSelectionChanged(
    WebContents* old_contents,
    NotifyTypes notify_types,
    const ui::ListSelectionModel& old_model) {
  NotifyIfActiveTabChanged(old_contents, notify_types);

  if (!selection_model().Equals(old_model)) {
    for (auto& observer : observers_)
      observer.TabSelectionChanged(this, old_model);
  }
}

void TabStripModel::SetSelection(
    const ui::ListSelectionModel& new_model,
    NotifyTypes notify_types) {
  WebContents* old_contents = GetActiveWebContents();
  ui::ListSelectionModel old_model;
  old_model.Copy(selection_model_);
  if (new_model.active() != selection_model_.active())
    NotifyIfTabDeactivated(old_contents);
  selection_model_.Copy(new_model);
  NotifyIfActiveOrSelectionChanged(old_contents, notify_types, old_model);
}

