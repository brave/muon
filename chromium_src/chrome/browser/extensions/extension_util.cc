// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_util.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/metrics/field_trial.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/sync_helper.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/site_instance.h"
#include "extensions/browser/disable_reason.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_icon_set.h"
#include "extensions/common/features/behavior_feature.h"
#include "extensions/common/features/feature.h"
#include "extensions/common/features/feature_provider.h"
#include "extensions/common/manifest.h"
#include "extensions/common/manifest_handlers/app_isolation_info.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/grit/extensions_browser_resources.h"
#include "ui/base/resource/resource_bundle.h"

namespace extensions {
namespace util {

namespace {

const char kSupervisedUserExtensionPermissionIncreaseFieldTrialName[] =
    "SupervisedUserExtensionPermissionIncrease";

// The entry into the prefs used to flag an extension as installed by custodian.
// It is relevant only for supervised users.
const char kWasInstalledByCustodianPrefName[] = "was_installed_by_custodian";

// Returns true if |extension| should always be enabled in incognito mode.
bool IsWhitelistedForIncognito(const Extension* extension) {
  const Feature* feature = FeatureProvider::GetBehaviorFeature(
      behavior_feature::kWhitelistedForIncognito);
  return feature && feature->IsAvailableToExtension(extension).is_available();
}

}  // namespace

void SetIsIncognitoEnabled(const std::string& extension_id,
                           content::BrowserContext* context,
                           bool enabled) {
  NOTIMPLEMENTED();
}

bool CanCrossIncognito(const Extension* extension,
                       content::BrowserContext* context) {
  // We allow the extension to see events and data from another profile iff it
  // uses "spanning" behavior and it has incognito access. "split" mode
  // extensions only see events for a matching profile.
  CHECK(extension);
  return IsIncognitoEnabled(extension->id(), context) &&
         !IncognitoInfo::IsSplitMode(extension);
}

bool CanLoadInIncognito(const Extension* extension,
                        content::BrowserContext* context) {
  CHECK(extension);
  if (extension->is_hosted_app())
    return true;
  // Packaged apps and regular extensions need to be enabled specifically for
  // incognito (and split mode should be set).
  return IncognitoInfo::IsSplitMode(extension) &&
         IsIncognitoEnabled(extension->id(), context);
}

bool AllowFileAccess(const std::string& extension_id,
                     content::BrowserContext* context) {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kDisableExtensionsFileAccessCheck) ||
         ExtensionPrefs::Get(context)->AllowFileAccess(extension_id);
}

void SetAllowFileAccess(const std::string& extension_id,
                        content::BrowserContext* context,
                        bool allow) {
  NOTIMPLEMENTED();
}

void SetWasInstalledByCustodian(const std::string& extension_id,
                                content::BrowserContext* context,
                                bool installed_by_custodian) {
  NOTIMPLEMENTED();
}

bool WasInstalledByCustodian(const std::string& extension_id,
                             content::BrowserContext* context) {
  bool installed_by_custodian = false;
  ExtensionPrefs* prefs = ExtensionPrefs::Get(context);
  prefs->ReadPrefAsBoolean(extension_id, kWasInstalledByCustodianPrefName,
                           &installed_by_custodian);
  return installed_by_custodian;
}

bool IsAppLaunchable(const std::string& extension_id,
                     content::BrowserContext* context) {
  int reason = ExtensionPrefs::Get(context)->GetDisableReasons(extension_id);
  return !((reason & disable_reason::DISABLE_UNSUPPORTED_REQUIREMENT) ||
           (reason & disable_reason::DISABLE_CORRUPTED));
}

bool IsAppLaunchableWithoutEnabling(const std::string& extension_id,
                                    content::BrowserContext* context) {
  return ExtensionRegistry::Get(context)->GetExtensionById(
      extension_id, ExtensionRegistry::ENABLED) != NULL;
}

bool ShouldSync(const Extension* extension,
                content::BrowserContext* context) {
  return sync_helper::IsSyncable(extension) &&
         !ExtensionPrefs::Get(context)->DoNotSync(extension->id());
}

bool IsExtensionIdle(const std::string& extension_id,
                     content::BrowserContext* context) {
  std::vector<std::string> ids_to_check;
  ids_to_check.push_back(extension_id);

  const Extension* extension =
      ExtensionRegistry::Get(context)
          ->GetExtensionById(extension_id, ExtensionRegistry::ENABLED);
  if (extension && extension->is_shared_module()) {
    // We have to check all the extensions that use this shared module for idle
    // to tell whether it is really 'idle'.
    NOTIMPLEMENTED();
  }

  ProcessManager* process_manager = ProcessManager::Get(context);
  for (std::vector<std::string>::const_iterator i = ids_to_check.begin();
       i != ids_to_check.end();
       i++) {
    const std::string id = (*i);
    ExtensionHost* host = process_manager->GetBackgroundHostForExtension(id);
    if (host)
      return false;

    scoped_refptr<content::SiteInstance> site_instance =
        process_manager->GetSiteInstanceForURL(
            Extension::GetBaseURLFromExtensionId(id));
    if (site_instance && site_instance->HasProcess())
      return false;

    if (!process_manager->GetRenderFrameHostsForExtension(id).empty())
      return false;
  }
  return true;
}

std::unique_ptr<base::DictionaryValue> GetExtensionInfo(
    const Extension* extension) {
  DCHECK(extension);
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);

  dict->SetString("id", extension->id());
  dict->SetString("name", extension->name());

  // TODO(bridiver)
  // GURL icon = extensions::ExtensionIconSource::GetIconURL(
  //     extension,
  //     extension_misc::EXTENSION_ICON_SMALLISH,
  //     ExtensionIconSet::MATCH_BIGGER,
  //     false,  // Not grayscale.
  //     NULL);  // Don't set bool if exists.
  // dict->SetString("icon", icon.spec());

  return dict;
}

const gfx::ImageSkia& GetDefaultAppIcon() {
  return *ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      IDR_APP_DEFAULT_ICON);
}

const gfx::ImageSkia& GetDefaultExtensionIcon() {
  return *ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
      IDR_EXTENSION_DEFAULT_ICON);
}

bool IsNewBookmarkAppsEnabled() {
#if defined(OS_MACOSX)
  return base::FeatureList::IsEnabled(features::kBookmarkApps);
#else
  return true;
#endif
}

bool CanHostedAppsOpenInWindows() {
#if defined(OS_MACOSX)
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableHostedAppsInWindows);
#else
  return true;
#endif
}

bool IsExtensionSupervised(const Extension* extension, Profile* profile) {
  NOTIMPLEMENTED();
  return false;
}

bool NeedCustodianApprovalForPermissionIncrease(const Profile* profile) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace util
}  // namespace extensions
