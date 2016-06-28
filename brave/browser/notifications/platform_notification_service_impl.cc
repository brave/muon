// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "brave/browser/notifications/platform_notification_service_impl.h"

#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/brave_permission_manager.h"
#include "browser/notification.h"
#include "browser/notification_delegate_adapter.h"
#include "browser/notification_presenter.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/platform_notification_data.h"
#include "content/public/common/notification_resources.h"
#include "third_party/skia/include/core/SkBitmap.h"


namespace brave {

namespace {

void OnPermissionResponse(const base::Callback<void(bool)>& callback,
                          blink::mojom::PermissionStatus status) {
  if (status == blink::mojom::PermissionStatus::GRANTED)
    callback.Run(true);
  else
    callback.Run(false);
}

void RemoveNotification(base::WeakPtr<brightray::Notification> notification) {
  if (notification)
    notification->Dismiss();
}

void OnWebNotificationAllowed(
    brightray::BrowserClient* browser_client,
    const SkBitmap& icon,
    const content::PlatformNotificationData& data,
    std::unique_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback,
    bool allowed) {
  if (!allowed)
    return;
  auto presenter = browser_client->GetNotificationPresenter();
  if (!presenter)
    return;
  std::unique_ptr<brightray::NotificationDelegateAdapter> adapter(
      new brightray::NotificationDelegateAdapter(std::move(delegate)));
  auto notification = presenter->CreateNotification(adapter.get());
  if (notification) {
    ignore_result(adapter.release());  // it will release itself automatically.
    notification->Show(data.title, data.body, data.tag, data.icon, icon, data.silent);
    if (cancel_callback)
      *cancel_callback = base::Bind(&RemoveNotification, notification);
  }
}

}  // namespace

// static
PlatformNotificationServiceImpl*
PlatformNotificationServiceImpl::GetInstance() {
  return base::Singleton<PlatformNotificationServiceImpl>::get();
}

PlatformNotificationServiceImpl::PlatformNotificationServiceImpl() {}

PlatformNotificationServiceImpl::~PlatformNotificationServiceImpl() {}

blink::WebNotificationPermission PlatformNotificationServiceImpl::CheckPermissionOnUIThread(
    content::BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  return blink::WebNotificationPermissionAllowed;
}

blink::WebNotificationPermission PlatformNotificationServiceImpl::CheckPermissionOnIOThread(
    content::ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  return blink::WebNotificationPermissionAllowed;
}

void PlatformNotificationServiceImpl::DisplayNotification(
    content::BrowserContext* browser_context,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources,
    std::unique_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {
  auto callback = base::Bind(&OnWebNotificationAllowed,
             BraveContentBrowserClient::Get(),
             notification_resources.notification_icon,
             notification_data,
             base::Passed(&delegate),
             cancel_callback);

  auto permission_manager = browser_context->GetPermissionManager();
  permission_manager->RequestPermission(
      content::PermissionType::NOTIFICATIONS, NULL, origin,
        base::Bind(&OnPermissionResponse, callback));
}

void PlatformNotificationServiceImpl::DisplayPersistentNotification(
    content::BrowserContext* browser_context,
    int64_t persistent_notification_id,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources) {
}

void PlatformNotificationServiceImpl::ClosePersistentNotification(
    content::BrowserContext* browser_context,
    int64_t persistent_notification_id) {
}

bool PlatformNotificationServiceImpl::GetDisplayedPersistentNotifications(
    content::BrowserContext* browser_context,
    std::set<std::string>* displayed_notifications) {
  return false;
}

}  // namespace brave
