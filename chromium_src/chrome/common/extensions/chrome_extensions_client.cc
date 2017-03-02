// Copyright (c) 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides Brave specific functionality, but is otherwise mostly
// a copy of the Chrome code. We put it here instead of creating a
// BraveExtensionsClient because the Chrome content renderer client wants this
// class name
#include "chrome/common/extensions/chrome_extensions_client.h"

#include <string>
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/grit/brave_resources.h"  // NOLINT: This file is generated
#include "brave/grit/brave_strings.h"  // NOLINT: This file is generated
#include "brave/common/extensions/api/generated_schemas.h"  // NOLINT: This file is generated
#include "brave/common/extensions/api/api_features.h"
#include "brave/common/extensions/api/behavior_features.h"
#include "brave/common/extensions/api/manifest_features.h"
#include "brave/common/extensions/api/permission_features.h"
#include "chrome/common/chrome_version.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "chrome/common/extensions/api/generated_schemas.h"  // NOLINT: This file is generated
#include "chrome/common/extensions/chrome_manifest_handlers.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/common_resources.h"  // NOLINT: This file is generated
#include "extensions/common/api/generated_schemas.h"  // NOLINT: This file is generated
#include "extensions/common/common_manifest_handlers.h"
#include "extensions/common/extension_api.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/extensions_aliases.h"
#include "extensions/common/features/json_feature_provider_source.h"
#include "chrome/common/extensions/chrome_aliases.h"
#include "extensions/common/manifest_handler.h"
#include "extensions/common/permissions/permission_message_provider.h"
#include "extensions/common/permissions/permissions_info.h"
#include "extensions/common/permissions/permissions_provider.h"
#include "extensions/common/url_pattern_set.h"
#include "extensions/grit/extensions_resources.h"  // NOLINT: This file is generated
#include "ui/base/l10n/l10n_util.h"


namespace extensions {

namespace {

const char kExtensionBlocklistUrlPrefix[] =
    "http://www.gstatic.com/chrome/extensions/blacklist";
const char kExtensionBlocklistHttpsUrlPrefix[] =
    "https://www.gstatic.com/chrome/extensions/blacklist";

static base::LazyInstance<ChromeExtensionsClient> g_client =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

ChromeExtensionsClient::ChromeExtensionsClient() {
}

ChromeExtensionsClient::~ChromeExtensionsClient() {
}

void ChromeExtensionsClient::Initialize() {
  if (!ManifestHandler::IsRegistrationFinalized()) {
    RegisterCommonManifestHandlers();
    RegisterChromeManifestHandlers();
    ManifestHandler::FinalizeRegistration();
  }

  // Set up permissions.
  PermissionsInfo::GetInstance()->AddProvider(chrome_api_permissions_,
                                              GetChromePermissionAliases());
  PermissionsInfo::GetInstance()->AddProvider(extensions_api_permissions_,
                                              GetExtensionsPermissionAliases());
}

const PermissionMessageProvider&
ChromeExtensionsClient::GetPermissionMessageProvider() const {
  return permission_message_provider_;
}

const std::string ChromeExtensionsClient::GetProductName() {
  return l10n_util::GetStringUTF8(IDS_PRODUCT_NAME);
}

std::unique_ptr<FeatureProvider> ChromeExtensionsClient::CreateFeatureProvider(
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
ChromeExtensionsClient::CreateAPIFeatureSource() const {
  std::unique_ptr<JSONFeatureProviderSource> source(
      new JSONFeatureProviderSource("api"));
  source->LoadJSON(IDR_EXTENSION_API_FEATURES);
  source->LoadJSON(IDR_BRAVE_API_FEATURES);
  return source;
}

void ChromeExtensionsClient::SetScriptingWhitelist(
    const ExtensionsClient::ScriptingWhitelist& whitelist) {
  scripting_whitelist_ = whitelist;
}

const ExtensionsClient::ScriptingWhitelist&
ChromeExtensionsClient::GetScriptingWhitelist() const {
  return scripting_whitelist_;
}

void ChromeExtensionsClient::FilterHostPermissions(
    const URLPatternSet& hosts,
    URLPatternSet* new_hosts,
    PermissionIDSet* permissions) const {
  for (URLPatternSet::const_iterator i = hosts.begin(); i != hosts.end(); ++i) {
    // Filters out every URL pattern that matches chrome:// scheme.
    if (i->scheme() == content::kChromeUIScheme) {
      // chrome://favicon is the only URL for chrome:// scheme that we
      // want to support. We want to deprecate the "chrome" scheme.
      // We should not add any additional "host" here.
      if (GURL(chrome::kChromeUIFaviconURL).host() != i->host())
        continue;
      permissions->insert(APIPermission::kFavicon);
    } else {
      new_hosts->AddPattern(*i);
    }
  }
}

URLPatternSet ChromeExtensionsClient::GetPermittedChromeSchemeHosts(
      const Extension* extension,
      const APIPermissionSet& api_permissions) const {
  URLPatternSet hosts;
  // Regular extensions are only allowed access to chrome://favicon.
  hosts.AddPattern(URLPattern(URLPattern::SCHEME_CHROMEUI,
                              chrome::kChromeUIFaviconURL));
  return hosts;
}

bool ChromeExtensionsClient::IsScriptableURL(
    const GURL& url, std::string* error) const {
  return true;
}

bool ChromeExtensionsClient::IsAPISchemaGenerated(
    const std::string& name) const {
  return api::BraveGeneratedSchemas::IsGenerated(name) ||
         api::ChromeGeneratedSchemas::IsGenerated(name) ||
         api::GeneratedSchemas::IsGenerated(name);
}

base::StringPiece ChromeExtensionsClient::GetAPISchema(
    const std::string& name) const {
  if (api::BraveGeneratedSchemas::IsGenerated(name))
    return api::BraveGeneratedSchemas::Get(name);

  // Test from most common to least common.
  if (api::ChromeGeneratedSchemas::IsGenerated(name))
    return api::ChromeGeneratedSchemas::Get(name);

  return api::GeneratedSchemas::Get(name);
}

bool ChromeExtensionsClient::ShouldSuppressFatalErrors() const {
  return true;
}

void ChromeExtensionsClient::RecordDidSuppressFatalError() {
}

const GURL& ChromeExtensionsClient::GetWebstoreBaseURL() const {
  webstore_update_url_ = GURL(chrome::kExtensionInvalidRequestURL);
  return webstore_update_url_;
}

const GURL& ChromeExtensionsClient::GetWebstoreUpdateURL() const {
  webstore_update_url_ = GURL(chrome::kExtensionInvalidRequestURL);
  return webstore_update_url_;
}

bool ChromeExtensionsClient::IsBlacklistUpdateURL(const GURL& url) const {
  return base::StartsWith(url.spec(), kExtensionBlocklistUrlPrefix,
                          base::CompareCase::SENSITIVE) ||
         base::StartsWith(url.spec(), kExtensionBlocklistHttpsUrlPrefix,
                          base::CompareCase::SENSITIVE);
}

std::set<base::FilePath> ChromeExtensionsClient::GetBrowserImagePaths(
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

// static
ChromeExtensionsClient* ChromeExtensionsClient::GetInstance() {
  return g_client.Pointer();
}

}  // namespace extensions
