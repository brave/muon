// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/platform_notification_service.h"

#include "browser/browser_client.h"
#include "browser/notification.h"
#include "browser/notification_delegate.h"
#include "browser/notification_presenter.h"
#include "content/public/common/platform_notification_data.h"
#include "content/public/common/notification_resources.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace brightray {

namespace {

void RemoveNotification(base::WeakPtr<Notification> notification) {
  if (notification)
    notification->Dismiss();
}

void OnWebNotificationAllowed(
    brightray::BrowserClient* browser_client,
    const SkBitmap& icon,
    const content::PlatformNotificationData& data,
    NotificationDelegate* delegate,
    base::Closure* cancel_callback,
    bool audio_muted,
    bool allowed) {
  if (!allowed)
    return;
  auto presenter = browser_client->GetNotificationPresenter();
  if (!presenter)
    return;
  auto notification = presenter->CreateNotification(delegate);
  if (notification) {
    notification->Show(data.title, data.body, data.tag, data.icon, icon,
                       audio_muted ? true : data.silent);
    *cancel_callback = base::Bind(&RemoveNotification, notification);
  }
}

}  // namespace

PlatformNotificationService::PlatformNotificationService(
    BrowserClient* browser_client)
    : browser_client_(browser_client),
      render_process_id_(-1) {
}

PlatformNotificationService::~PlatformNotificationService() {}

blink::mojom::PermissionStatus PlatformNotificationService::CheckPermissionOnUIThread(
    content::BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  render_process_id_ = render_process_id;
  return blink::mojom::PermissionStatus::GRANTED;
}

blink::mojom::PermissionStatus PlatformNotificationService::CheckPermissionOnIOThread(
    content::ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  return blink::mojom::PermissionStatus::GRANTED;
}

void PlatformNotificationService::DisplayNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources,
    base::Closure* cancel_callback) {
  NotificationDelegate* delegate =
      new NotificationDelegate(notification_id);
  browser_client_->WebNotificationAllowed(
      render_process_id_,
      base::Bind(&OnWebNotificationAllowed,
                 browser_client_,
                 notification_resources.notification_icon,
                 notification_data,
                 base::Passed(&delegate),
                 cancel_callback));
}

void PlatformNotificationService::DisplayPersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const GURL& service_worker_origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources) {
}

void PlatformNotificationService::ClosePersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id) {
}

void PlatformNotificationService::GetDisplayedNotifications(
    content::BrowserContext* browser_context,
    const DisplayedNotificationsCallback& callback) {
}

}  // namespace brightray
