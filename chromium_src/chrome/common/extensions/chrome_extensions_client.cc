// Copyright (c) 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides Brave specific functionality, but is otherwise mostly
// a copy of the Chrome code. We put it here instead of creating a
// BraveExtensionsClient because the Chrome content renderer client wants this
// class name
#include "chrome/common/extensions/chrome_extensions_client.h"

#include <string>
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/grit/brave_resources.h"  // NOLINT: This file is generated
#include "brave/grit/brave_strings.h"  // NOLINT: This file is generated
#include "brave/common/extensions/api/generated_schemas.h"  // NOLINT: This file is generated
#include "brave/common/extensions/api/api_features.h"
#include "brave/common/extensions/api/behavior_features.h"
#include "brave/common/extensions/api/manifest_features.h"
#include "brave/common/extensions/api/permission_features.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/chrome_version.h"
#include "chrome/common/extensions/api/extension_action/action_info.h"
#include "chrome/common/extensions/api/generated_schemas.h"  // NOLINT: This file is generated
#include "chrome/common/extensions/chrome_manifest_handlers.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/manifest_handlers/theme_handler.h"
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

// TODO(battre): Delete the HTTP URL once the blacklist is downloaded via HTTPS.
const char kExtensionBlocklistUrlPrefix[] =
    "http://www.gstatic.com/chrome/extensions/blacklist";
const char kExtensionBlocklistHttpsUrlPrefix[] =
    "https://www.gstatic.com/chrome/extensions/blacklist";

}  // namespace

static base::LazyInstance<ChromeExtensionsClient>::Leaky g_client =
    LAZY_INSTANCE_INITIALIZER;

ChromeExtensionsClient::ChromeExtensionsClient() {}

ChromeExtensionsClient::~ChromeExtensionsClient() {
}

void ChromeExtensionsClient::Initialize() {
  // Registration could already be finalized in unit tests, where the utility
  // thread runs in-process.
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
  InitializeWebStoreUrls(base::CommandLine::ForCurrentProcess());
}

void ChromeExtensionsClient::InitializeWebStoreUrls(
    base::CommandLine* command_line) {
  if (command_line->HasSwitch(switches::kAppsGalleryURL)) {
    webstore_base_url_ =
        GURL(command_line->GetSwitchValueASCII(switches::kAppsGalleryURL));
  } else {
    webstore_base_url_ = GURL(extension_urls::kChromeWebstoreBaseURL);
  }
  if (command_line->HasSwitch(switches::kAppsGalleryUpdateURL)) {
    webstore_update_url_ = GURL(
        command_line->GetSwitchValueASCII(switches::kAppsGalleryUpdateURL));
  } else {
    webstore_update_url_ = GURL(extension_urls::GetDefaultWebstoreUpdateUrl());
  }
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

void ChromeExtensionsClient::FilterHostPermissions(
    const URLPatternSet& hosts,
    URLPatternSet* new_hosts,
    PermissionIDSet* permissions) const {
  // When editing this function, be sure to add the same functionality to
  // FilterHostPermissions() above.
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

void ChromeExtensionsClient::SetScriptingWhitelist(
    const ExtensionsClient::ScriptingWhitelist& whitelist) {
  scripting_whitelist_ = whitelist;
}

const ExtensionsClient::ScriptingWhitelist&
ChromeExtensionsClient::GetScriptingWhitelist() const {
  return scripting_whitelist_;
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
  // Test from most common to least common.
  return api::BraveGeneratedSchemas::IsGenerated(name) ||
         api::ChromeGeneratedSchemas::IsGenerated(name) ||
         api::GeneratedSchemas::IsGenerated(name);
}

base::StringPiece ChromeExtensionsClient::GetAPISchema(
    const std::string& name) const {
  base::StringPiece brave_schema = api::BraveGeneratedSchemas::Get(name);
  if (!brave_schema.empty())
    return brave_schema;

  // Test from most common to least common.
  base::StringPiece chrome_schema = api::ChromeGeneratedSchemas::Get(name);
  if (!chrome_schema.empty())
    return chrome_schema;

  return api::GeneratedSchemas::Get(name);
}

bool ChromeExtensionsClient::ShouldSuppressFatalErrors() const {
  // Suppress fatal everywhere until the cause of bugs like http://crbug/471599
  // are fixed. This would typically be:
  // return GetCurrentChannel() > version_info::Channel::DEV;
  return true;
}

void ChromeExtensionsClient::RecordDidSuppressFatalError() {
}

const GURL& ChromeExtensionsClient::GetWebstoreBaseURL() const {
  return webstore_base_url_;
}

const GURL& ChromeExtensionsClient::GetWebstoreUpdateURL() const {
  return webstore_update_url_;
}

bool ChromeExtensionsClient::IsBlacklistUpdateURL(const GURL& url) const {
  // The extension blacklist URL is returned from the update service and
  // therefore not determined by Chromium. If the location of the blacklist file
  // ever changes, we need to update this function. A DCHECK in the
  // ExtensionUpdater ensures that we notice a change. This is the full URL
  // of a blacklist:
  // http://www.gstatic.com/chrome/extensions/blacklist/l_0_0_0_7.txt
  return base::StartsWith(url.spec(), kExtensionBlocklistUrlPrefix,
                          base::CompareCase::SENSITIVE) ||
         base::StartsWith(url.spec(), kExtensionBlocklistHttpsUrlPrefix,
                          base::CompareCase::SENSITIVE);
}

std::set<base::FilePath> ChromeExtensionsClient::GetBrowserImagePaths(
    const Extension* extension) {
  std::set<base::FilePath> image_paths =
      ExtensionsClient::GetBrowserImagePaths(extension);

  // Theme images
  const base::DictionaryValue* theme_images = ThemeInfo::GetImages(extension);
  if (theme_images) {
    for (base::DictionaryValue::Iterator it(*theme_images); !it.IsAtEnd();
         it.Advance()) {
      base::FilePath::StringType path;
      if (it.value().GetAsString(&path))
        image_paths.insert(base::FilePath(path));
    }
  }

  const ActionInfo* page_action = ActionInfo::GetPageActionInfo(extension);
  if (page_action && !page_action->default_icon.empty())
    page_action->default_icon.GetPaths(&image_paths);

  const ActionInfo* browser_action =
      ActionInfo::GetBrowserActionInfo(extension);
  if (browser_action && !browser_action->default_icon.empty())
    browser_action->default_icon.GetPaths(&image_paths);

  return image_paths;
}

bool ChromeExtensionsClient::ExtensionAPIEnabledInExtensionServiceWorkers()
    const {
  return false;
}

std::string ChromeExtensionsClient::GetUserAgent() const {
  return ::GetUserAgent();
}

// static
ChromeExtensionsClient* ChromeExtensionsClient::GetInstance() {
  return g_client.Pointer();
}

}  // namespace extensions
