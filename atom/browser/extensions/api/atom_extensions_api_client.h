// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_API_ATOM_EXTENSIONS_API_CLIENT_H_
#define ATOM_BROWSER_EXTENSIONS_API_ATOM_EXTENSIONS_API_CLIENT_H_

#include "extensions/browser/api/extensions_api_client.h"

namespace extensions {

class ManagementAPIDelegate;

class AtomExtensionsAPIClient : public ExtensionsAPIClient {
 public:
  AtomExtensionsAPIClient();

  // ExtensionsAPIClient implementation.
  void AttachWebContentsHelpers(content::WebContents* web_contents) const
      override;
  std::unique_ptr<guest_view::GuestViewManagerDelegate>
      CreateGuestViewManagerDelegate(
          content::BrowserContext* context) const;
  std::unique_ptr<WebRequestEventRouterDelegate>
      CreateWebRequestEventRouterDelegate() const override;
  ManagementAPIDelegate* CreateManagementAPIDelegate() const override;
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_API_ATOM_EXTENSIONS_API_CLIENT_H_
