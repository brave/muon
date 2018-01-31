// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_PERMISSION_MANAGER_H_
#define BRAVE_BROWSER_BRAVE_PERMISSION_MANAGER_H_

#include <map>
#include <vector>

#include "base/callback.h"
#include "content/public/browser/permission_manager.h"
#include "services/device/public/interfaces/geolocation_control.mojom.h"

namespace content {
class WebContents;
}

namespace brave {
class BravePermissionManager : public content::PermissionManager {
 public:
  BravePermissionManager();
  ~BravePermissionManager() override;

  using ResponseCallback =
      base::Callback<void(
          const std::vector<blink::mojom::PermissionStatus>& status)>;
  using RequestHandler =
      base::Callback<void(const GURL&,
                      const GURL&,
                      const std::vector<content::PermissionType>& permissions,
                      const ResponseCallback&)>;

  // Handler to dispatch permission requests in JS.
  void SetPermissionRequestHandler(const RequestHandler& handler);

  // content::PermissionManager:
  int RequestPermission(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(
          blink::mojom::PermissionStatus)>& callback) override;
  int RequestPermissions(
      const std::vector<content::PermissionType>& permissions,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(
      const std::vector<blink::mojom::PermissionStatus>&)>& callback) override;

 protected:
  void OnPermissionResponse(int request_id,
      const GURL& url,
      const ResponseCallback& callback,
      const std::vector<content::PermissionType>& permissions,
      const std::vector<blink::mojom::PermissionStatus>& status);

  // content::PermissionManager:
  void CancelPermissionRequest(int request_id) override;
  void ResetPermission(content::PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatus(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin) override;
  int SubscribePermissionStatusChange(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin,
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback)
      override;
  void UnsubscribePermissionStatusChange(int subscription_id) override;

 private:
  device::mojom::GeolocationControl* GetGeolocationControl();

  struct RequestInfo {
    int render_process_id;
    int render_frame_id;
    ResponseCallback callback;
    size_t size;
  };

  RequestHandler request_handler_;

  std::map<int, RequestInfo> pending_requests_;

  int request_id_;

  device::mojom::GeolocationControlPtr geolocation_control_;

  DISALLOW_COPY_AND_ASSIGN(BravePermissionManager);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_PERMISSION_MANAGER_H_
