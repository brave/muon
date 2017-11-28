// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/content_settings/host_content_settings_map_factory.h"

#include <utility>

// #include "chrome/browser/profiles/off_the_record_profile_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/features/features.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_extension_system.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"
#endif

HostContentSettingsMapFactory::HostContentSettingsMapFactory()
    : RefcountedBrowserContextKeyedServiceFactory(
        "HostContentSettingsMap",
        BrowserContextDependencyManager::GetInstance()) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DependsOn(
      extensions::ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
#endif
}

HostContentSettingsMapFactory::~HostContentSettingsMapFactory() {
}

// static
HostContentSettingsMap* HostContentSettingsMapFactory::GetForProfile(
    Profile* profile) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  return static_cast<HostContentSettingsMap*>(
      GetInstance()->GetServiceForBrowserContext(profile, true).get());
}

// static
HostContentSettingsMapFactory* HostContentSettingsMapFactory::GetInstance() {
  return base::Singleton<HostContentSettingsMapFactory>::get();
}

scoped_refptr<RefcountedKeyedService>
    HostContentSettingsMapFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Profile* profile = static_cast<Profile*>(context);

  // If off the record, retrieve the host content settings map of the parent
  // profile in order to ensure the preferences have been migrated.
  if (profile->IsOffTheRecord())
    GetForProfile(profile->GetOriginalProfile());

  scoped_refptr<HostContentSettingsMap> settings_map(new HostContentSettingsMap(
      profile->GetPrefs(), profile->IsOffTheRecord(), false, false));

#if BUILDFLAG(ENABLE_EXTENSIONS)
  ExtensionService *ext_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  // This may be null in testing or when the extenion_service hasn't been
  // initialized, in which case it will be registered then.
  if (ext_service)
    ext_service->RegisterContentSettings(settings_map.get());
#endif // BUILDFLAG(ENABLE_EXTENSIONS)
  return settings_map;
}

content::BrowserContext* HostContentSettingsMapFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);

  if (profile->IsOffTheRecord())
    return profile;

  return profile->GetOriginalProfile();
}
