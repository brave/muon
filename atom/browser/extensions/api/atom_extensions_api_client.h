// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_API_ATOM_EXTENSIONS_API_CLIENT_H_
#define ATOM_BROWSER_EXTENSIONS_API_ATOM_EXTENSIONS_API_CLIENT_H_

#include <map>
#include <memory>

#include "extensions/browser/api/extensions_api_client.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class ManagementAPIDelegate;
class SettingsObserver;
class ValueStoreCache;
class ValueStoreFactory;

class AtomExtensionsAPIClient : public ExtensionsAPIClient {
 public:
  AtomExtensionsAPIClient();

  // ExtensionsAPIClient implementation.
  void AttachWebContentsHelpers(content::WebContents* web_contents) const
      override;
  std::unique_ptr<guest_view::GuestViewManagerDelegate>
  CreateGuestViewManagerDelegate(
      content::BrowserContext* context) const override;
  std::unique_ptr<WebRequestEventRouterDelegate>
      CreateWebRequestEventRouterDelegate() const override;
  ManagementAPIDelegate* CreateManagementAPIDelegate() const override;
  void AddAdditionalValueStoreCaches(
      content::BrowserContext* context,
      const scoped_refptr<ValueStoreFactory>& factory,
      const scoped_refptr<base::ObserverListThreadSafe<SettingsObserver>>&
          observers,
      std::map<settings_namespace::Namespace,
          ValueStoreCache*>* caches) override;
  MessagingDelegate* GetMessagingDelegate() override;

 private:
  std::unique_ptr<MessagingDelegate> messaging_delegate_;
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_API_ATOM_EXTENSIONS_API_CLIENT_H_
