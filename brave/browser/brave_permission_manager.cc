// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/browser/brave_permission_manager.h"

#include <utility>

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/service_manager_connection.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace brave {

namespace {

bool WebContentsDestroyed(int render_process_id, int render_frame_id) {
  if (render_process_id == MSG_ROUTING_NONE)
    return false;

  auto host = content::RenderProcessHost::FromID(render_process_id);
  if (!host)
    return true;

  auto rfh =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  if (!rfh)
    return true;

  auto contents =
      content::WebContents::FromRenderFrameHost(rfh);
  if (!contents)
    return true;

  return contents->IsBeingDestroyed();
}

void PermissionStatusCallbackWrapper(const base::Callback<void(
                    blink::mojom::PermissionStatus)>& callback,
                    const std::vector<blink::mojom::PermissionStatus>& status) {
  callback.Run(status.front());
}

}  // namespace

BravePermissionManager::BravePermissionManager()
    : request_id_(0) {
}

BravePermissionManager::~BravePermissionManager() {
}

void BravePermissionManager::SetPermissionRequestHandler(
    const RequestHandler& handler) {
  if (handler.is_null() && !pending_requests_.empty()) {
    for (const auto& request : pending_requests_)
      CancelPermissionRequest(request.first);
    pending_requests_.clear();
  }
  request_handler_ = handler;
}

int BravePermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<void(
          blink::mojom::PermissionStatus)>& response_callback) {
  auto callback =
      base::Bind(PermissionStatusCallbackWrapper, response_callback);
  return RequestPermissions({ permission }, render_frame_host,
      requesting_origin, user_gesture, callback);
}

int BravePermissionManager::RequestPermissions(
    const std::vector<content::PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<void(
    const std::vector<blink::mojom::PermissionStatus>&)>& response_callback) {
  int render_frame_id = MSG_ROUTING_NONE;
  int render_process_id = MSG_ROUTING_NONE;
  GURL url;
  // web notifications do not currently have an available render_frame_host
  if (render_frame_host) {
    render_process_id = render_frame_host->GetProcess()->GetID();

    content::WebContents* web_contents =
        content::WebContents::FromRenderFrameHost(render_frame_host);

    if (web_contents) {
      render_frame_id = render_frame_host->GetRoutingID();
      url = web_contents->GetURL();
    }
  }
  std::vector<blink::mojom::PermissionStatus> permissionStatuses;
  for (auto permission : permissions) {
    if (permission == content::PermissionType::MIDI_SYSEX) {
      content::ChildProcessSecurityPolicy::GetInstance()->
          GrantSendMidiSysExMessage(render_frame_host->GetProcess()->GetID());
    }
    permissionStatuses.push_back(blink::mojom::PermissionStatus::GRANTED);
  }

  if (!request_handler_.is_null()) {
    ++request_id_;
    auto callback = base::Bind(&BravePermissionManager::OnPermissionResponse,
                               base::Unretained(this),
                               request_id_,
                               requesting_origin,
                               response_callback,
                               permissions);
    pending_requests_[request_id_] =
        { render_process_id, render_frame_id, callback, permissions.size() };
    request_handler_.Run(requesting_origin, url, permissions, callback);
    return request_id_;
  }

  response_callback.Run(permissionStatuses);
  return kNoPendingOperation;
}

device::mojom::GeolocationControl*
BravePermissionManager::GetGeolocationControl() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (geolocation_control_)
    return geolocation_control_.get();

  auto request = mojo::MakeRequest(&geolocation_control_);
  if (!content::ServiceManagerConnection::GetForProcess())
    return geolocation_control_.get();

  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  connector->BindInterface(device::mojom::kServiceName, std::move(request));
  return geolocation_control_.get();
}

void BravePermissionManager::OnPermissionResponse(
    int request_id,
    const GURL& origin,
    const ResponseCallback& callback,
    const std::vector<content::PermissionType>& permissions,
    const std::vector<blink::mojom::PermissionStatus>& status) {
  auto request = pending_requests_.find(request_id);
  if (request != pending_requests_.end()) {
    if (!WebContentsDestroyed(
        request->second.render_process_id, request->second.render_frame_id)) {
      for (int i = 0; i < permissions.size(); i++) {
        if (permissions[i] == content::PermissionType::GEOLOCATION) {
          if (status[i] == blink::mojom::PermissionStatus::GRANTED) {
            GetGeolocationControl()->UserDidOptIntoLocationServices();
          }
        }
      }
      callback.Run(status);
    }
    pending_requests_.erase(request);
  }
}

void BravePermissionManager::CancelPermissionRequest(int request_id) {
  auto request = pending_requests_.find(request_id);
  if (request != pending_requests_.end()) {
    if (!WebContentsDestroyed(
        request->second.render_process_id, request->second.render_frame_id)) {
      std::vector<blink::mojom::PermissionStatus> permissionStatuses;
      for (size_t i = 0; i < request->second.size; i++) {
        permissionStatuses.push_back(blink::mojom::PermissionStatus::DENIED);
      }
      request->second.callback.Run(permissionStatuses);
      // We should not erase from the map here because the callback which calls
      // BravePermissionManager::OnPermissionResponse will remove.
    } else {
      pending_requests_.erase(request);
    }
  }
}

void BravePermissionManager::ResetPermission(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

blink::mojom::PermissionStatus BravePermissionManager::GetPermissionStatus(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return blink::mojom::PermissionStatus::GRANTED;
}

int BravePermissionManager::SubscribePermissionStatusChange(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback) {
  return -1;
}

void BravePermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {
}

}  // namespace brave
