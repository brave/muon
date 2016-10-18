// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/extensions/atom_extensions_client.h"

#include <string>
#include "brave/grit/brave_resources.h"  // NOLINT: This file is generated
#include "brave/common/extensions/api/generated_schemas.h"  // NOLINT: This file is generated
#include "chrome/common/chrome_version.h"
#include "chrome/common/extensions/api/generated_schemas.h"  // NOLINT: This file is generated
#include "chrome/common/extensions/manifest_handlers/content_scripts_handler.h"
#include "chrome/grit/common_resources.h"  // NOLINT: This file is generated
#include "brave/common/extensions/api/api_features.h"
#include "brave/common/extensions/api/behavior_features.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "chrome/common/extensions/api/generated_schemas.h"
#include "brave/common/extensions/api/manifest_features.h"
#include "brave/common/extensions/api/permission_features.h"
#include "extensions/common/api/generated_schemas.h"  // NOLINT: This file is generated
#include "extensions/common/common_manifest_handlers.h"
#include "extensions/common/extension_api.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/features/api_feature.h"
#include "extensions/common/features/behavior_feature.h"
#include "extensions/common/features/json_feature_provider_source.h"
#include "extensions/common/features/manifest_feature.h"
#include "extensions/common/features/permission_feature.h"
#include "extensions/common/features/simple_feature.h"
#include "extensions/common/manifest_handler.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/permissions/permissions_info.h"
#include "extensions/common/permissions/permissions_provider.h"
#include "extensions/common/url_pattern_set.h"
#include "extensions/grit/extensions_resources.h"  // NOLINT: This file is generated


namespace extensions {

namespace {

template <class FeatureClass>
SimpleFeature* CreateFeature() {
  return new FeatureClass;
}

class AtomPermissionMessageProvider : public PermissionMessageProvider {
 public:
  // TODO(bridiver) implement this before allowing users to install extensions
  AtomPermissionMessageProvider() {}
  ~AtomPermissionMessageProvider() override {}

  // PermissionMessageProvider implementation.
  PermissionMessages GetPermissionMessages(
      const PermissionIDSet& permissions) const override {
    return PermissionMessages();
  }

  bool IsPrivilegeIncrease(const PermissionSet& old_permissions,
                           const PermissionSet& new_permissions,
                           Manifest::Type extension_type) const override {
    CHECK(false);
    return false;
  }

  PermissionIDSet GetAllPermissionIDs(
      const PermissionSet& permissions,
      Manifest::Type extension_type) const override {
    return PermissionIDSet();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomPermissionMessageProvider);
};

base::LazyInstance<AtomPermissionMessageProvider>
    g_permission_message_provider = LAZY_INSTANCE_INITIALIZER;

static base::LazyInstance<AtomExtensionsClient> g_client =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

AtomExtensionsClient::AtomExtensionsClient()
    : chrome_api_permissions_(ChromeAPIPermissions()),
      extensions_api_permissions_(ExtensionsAPIPermissions()) {
}

AtomExtensionsClient::~AtomExtensionsClient() {
}

void AtomExtensionsClient::Initialize() {
  (new ContentScriptsHandler)->Register();
  RegisterCommonManifestHandlers();
  ManifestHandler::FinalizeRegistration();

  PermissionsInfo::GetInstance()->AddProvider(chrome_api_permissions_);
  PermissionsInfo::GetInstance()->AddProvider(extensions_api_permissions_);
}

const PermissionMessageProvider&
AtomExtensionsClient::GetPermissionMessageProvider() const {
  NOTIMPLEMENTED();
  return g_permission_message_provider.Get();
}

const std::string AtomExtensionsClient::GetProductName() {
  return PRODUCT_SHORTNAME_STRING;
}

std::unique_ptr<FeatureProvider> AtomExtensionsClient::CreateFeatureProvider(
    const std::string& name) const {
  std::unique_ptr<FeatureProvider> provider;
  if (name == "api") {
    provider.reset(new BraveAPIFeatureProvider());
  } else if (name == "manifest") {
    provider.reset(new BraveManifestFeatureProvider());
  } else if (name == "permission") {
    provider.reset(new BravePermissionFeatureProvider());
  } else if (name == "behavior") {
    provider.reset(new BraveBehaviorFeatureProvider());
  } else {
    NOTREACHED();
  }
  return provider;
}

std::unique_ptr<JSONFeatureProviderSource>
AtomExtensionsClient::CreateAPIFeatureSource() const {
  std::unique_ptr<JSONFeatureProviderSource> source(
      new JSONFeatureProviderSource("api"));
  source->LoadJSON(IDR_EXTENSION_API_FEATURES);
  source->LoadJSON(IDR_BRAVE_API_FEATURES);
  return source;
}

std::set<base::FilePath> AtomExtensionsClient::GetBrowserImagePaths(
    const Extension* extension) {
  std::set<base::FilePath> image_paths =
      ExtensionsClient::GetBrowserImagePaths(extension);

  const ActionInfo* page_action = ActionInfo::GetPageActionInfo(extension);
  if (page_action && !page_action->default_icon.empty())
    page_action->default_icon.GetPaths(&image_paths);

  const ActionInfo* browser_action =
      ActionInfo::GetBrowserActionInfo(extension);
  if (browser_action && !browser_action->default_icon.empty())
    browser_action->default_icon.GetPaths(&image_paths);

  return image_paths;
}

void AtomExtensionsClient::FilterHostPermissions(
    const URLPatternSet& hosts,
    URLPatternSet* new_hosts,
    PermissionIDSet* permissions) const {
  NOTIMPLEMENTED();
}

void AtomExtensionsClient::SetScriptingWhitelist(
    const ExtensionsClient::ScriptingWhitelist& whitelist) {
  scripting_whitelist_ = whitelist;
}

const ExtensionsClient::ScriptingWhitelist&
AtomExtensionsClient::GetScriptingWhitelist() const {
  return scripting_whitelist_;
}

URLPatternSet AtomExtensionsClient::GetPermittedChromeSchemeHosts(
      const Extension* extension,
      const APIPermissionSet& api_permissions) const {
  return URLPatternSet();
}

bool AtomExtensionsClient::IsScriptableURL(
    const GURL& url, std::string* error) const {
  return true;
}

bool AtomExtensionsClient::IsAPISchemaGenerated(
    const std::string& name) const {
  return api::BraveGeneratedSchemas::IsGenerated(name) ||
         api::ChromeGeneratedSchemas::IsGenerated(name) ||
         api::GeneratedSchemas::IsGenerated(name);
}

base::StringPiece AtomExtensionsClient::GetAPISchema(
    const std::string& name) const {
  if (api::BraveGeneratedSchemas::IsGenerated(name))
    return api::BraveGeneratedSchemas::Get(name);

  // Test from most common to least common.
  if (api::ChromeGeneratedSchemas::IsGenerated(name))
    return api::ChromeGeneratedSchemas::Get(name);

  return api::GeneratedSchemas::Get(name);
}

bool AtomExtensionsClient::ShouldSuppressFatalErrors() const {
  return true;
}

void AtomExtensionsClient::RecordDidSuppressFatalError() {
}

std::string AtomExtensionsClient::GetWebstoreBaseURL() const {
  return "about:blank";
}

std::string AtomExtensionsClient::GetWebstoreUpdateURL() const {
  return "about:blank";
}

bool AtomExtensionsClient::IsBlacklistUpdateURL(const GURL& url) const {
  return false;
}

// static
AtomExtensionsClient* AtomExtensionsClient::GetInstance() {
  return g_client.Pointer();
}

}  // namespace extensions
