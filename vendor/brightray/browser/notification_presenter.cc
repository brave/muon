// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "browser/notification_delegate.h"
#include "browser/notification_presenter.h"

#include "browser/notification.h"

namespace brightray {

NotificationPresenter::NotificationPresenter() {
}

NotificationPresenter::~NotificationPresenter() {
  for (Notification* notification : notifications_)
    delete notification;
}

base::WeakPtr<Notification> NotificationPresenter::CreateNotification(
    NotificationDelegate* delegate) {
  Notification* notification = Notification::Create(delegate, this);
  notifications_.insert(notification);
  return notification->GetWeakPtr();
}

void NotificationPresenter::RemoveNotification(Notification* notification) {
  notifications_.erase(notification);
  delete notification;
}

Notification* NotificationPresenter::lookupNotification(
    const std::string& notification_id) const {
  for (auto notification : notifications_) {
    if (notification->delegate_->notificationId() == notification_id)
      return notification;
  }
  return nullptr;
}

}  // namespace brightray
