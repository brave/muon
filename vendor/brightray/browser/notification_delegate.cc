// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/bind_helpers.h"
#include "brightray/browser/notification_delegate.h"
#include "content/public/browser/notification_event_dispatcher.h"

namespace brightray {

NotificationDelegate::NotificationDelegate(std::string notification_id)
    : notification_id_(notification_id) {
}

void NotificationDelegate::NotificationDestroyed() {
  delete this;
}

void NotificationDelegate::NotificationClick() {
  content::NotificationEventDispatcher::GetInstance()
      ->DispatchNonPersistentClickEvent(notification_id_);
}

void NotificationDelegate::NotificationClosed() {
  content::NotificationEventDispatcher::GetInstance()
      ->DispatchNonPersistentCloseEvent(notification_id_, base::DoNothing());
}

void NotificationDelegate::NotificationDisplayed() {
  content::NotificationEventDispatcher::GetInstance()
      ->DispatchNonPersistentShowEvent(notification_id_);
}

}  // namespace brightray

