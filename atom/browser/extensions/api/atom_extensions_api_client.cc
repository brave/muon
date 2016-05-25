// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/api/atom_extensions_api_client.h"

#include "atom/browser/extensions/atom_extension_web_contents_observer.h"
#include "atom/browser/extensions/extension_renderer_state.h"
#include "atom/browser/extensions/tab_helper.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/api/web_request/web_request_event_details.h"
#include "extensions/browser/api/web_request/web_request_event_router_delegate.h"

namespace extensions {

class AtomExtensionWebRequestEventRouterDelegate :
    public WebRequestEventRouterDelegate {
 public:
  AtomExtensionWebRequestEventRouterDelegate() {}
  ~AtomExtensionWebRequestEventRouterDelegate() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomExtensionWebRequestEventRouterDelegate);
};

AtomExtensionsAPIClient::AtomExtensionsAPIClient() {
}

void AtomExtensionsAPIClient::AttachWebContentsHelpers(
    content::WebContents* web_contents) const {
  extensions::TabHelper::CreateForWebContents(web_contents);
  AtomExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

WebViewGuestDelegate* AtomExtensionsAPIClient::CreateWebViewGuestDelegate(
    WebViewGuest* web_view_guest) const {
  return ExtensionsAPIClient::CreateWebViewGuestDelegate(web_view_guest);
}

WebRequestEventRouterDelegate*
AtomExtensionsAPIClient::CreateWebRequestEventRouterDelegate() const {
  return new AtomExtensionWebRequestEventRouterDelegate();
}

}  // namespace extensions
