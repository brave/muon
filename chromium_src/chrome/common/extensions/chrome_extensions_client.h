// Copyright (c) 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_CHROME_EXTENSIONS_CLIENT_H_
#define CHROME_COMMON_EXTENSIONS_CHROME_EXTENSIONS_CLIENT_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "chrome/common/extensions/permissions/chrome_api_permissions.h"
#include "chrome/common/extensions/permissions/chrome_permission_message_provider.h"
#include "extensions/common/extensions_client.h"
#include "extensions/common/permissions/extensions_api_permissions.h"
#include "url/gurl.h"

namespace extensions {

class ChromeExtensionsClient : public ExtensionsClient {
 public:
  ChromeExtensionsClient();
  ~ChromeExtensionsClient() override;

  void Initialize() override;

  const PermissionMessageProvider& GetPermissionMessageProvider()
      const override;
  const std::string GetProductName() override;
  std::unique_ptr<FeatureProvider> CreateFeatureProvider(
      const std::string& name) const override;
  std::unique_ptr<JSONFeatureProviderSource> CreateAPIFeatureSource()
      const override;
  void FilterHostPermissions(const URLPatternSet& hosts,
                             URLPatternSet* new_hosts,
                             PermissionIDSet* permissions) const override;
  void SetScriptingWhitelist(const ScriptingWhitelist& whitelist) override;
  const ScriptingWhitelist& GetScriptingWhitelist() const override;
  URLPatternSet GetPermittedChromeSchemeHosts(
      const Extension* extension,
      const APIPermissionSet& api_permissions) const override;
  bool IsScriptableURL(const GURL& url, std::string* error) const override;
  bool IsAPISchemaGenerated(const std::string& name) const override;
  base::StringPiece GetAPISchema(const std::string& name) const override;
  bool ShouldSuppressFatalErrors() const override;
  void RecordDidSuppressFatalError() override;
  const GURL& GetWebstoreBaseURL() const override;
  const GURL& GetWebstoreUpdateURL() const override;
  bool IsBlacklistUpdateURL(const GURL& url) const override;
  std::set<base::FilePath> GetBrowserImagePaths(
    const Extension* extension);

  // Get the LazyInstance for ChromeExtensionsClient.
  static ChromeExtensionsClient* GetInstance();

 private:
  const ChromeAPIPermissions chrome_api_permissions_;
  const ExtensionsAPIPermissions extensions_api_permissions_;
  const ChromePermissionMessageProvider permission_message_provider_;

  // A whitelist of extensions that can script anywhere. Do not add to this
  // list (except in tests) without consulting the Extensions team first.
  // Note: Component extensions have this right implicitly and do not need to be
  // added to this list.
  ScriptingWhitelist scripting_whitelist_;

  // Mutable to allow caching in a const method.
  mutable GURL webstore_update_url_;

  friend struct base::DefaultLazyInstanceTraits<ChromeExtensionsClient>;
  DISALLOW_COPY_AND_ASSIGN(ChromeExtensionsClient);
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_CHROME_EXTENSIONS_CLIENT_H_
