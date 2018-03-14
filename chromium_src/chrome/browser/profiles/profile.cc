// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile.h"

#include <string>

#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "extensions/features/features.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/pref_names.h"
#endif

Profile::Profile(const std::string& partition, bool in_memory,
                     const base::DictionaryValue& options)
    : atom::AtomBrowserContext(partition, in_memory, options),
      // restored_last_session_(false),
      sent_destroyed_notification_(false) {
      // accessibility_pause_level_(0),
      // is_guest_profile_(false),
      // is_system_profile_(false) {
}

Profile::~Profile() {
}

Profile::Delegate::~Delegate() {
}

// static
Profile* Profile::FromBrowserContext(content::BrowserContext* browser_context) {
  // This is safe; this is the only implementation of the browser context.
  return static_cast<Profile*>(browser_context);
}

// static
Profile* Profile::FromWebUI(content::WebUI* web_ui) {
  return FromBrowserContext(web_ui->GetWebContents()->GetBrowserContext());
}

ChromeZoomLevelPrefs* Profile::GetZoomLevelPrefs() {
  return NULL;
}

void Profile::MaybeSendDestroyedNotification() {
  if (!sent_destroyed_notification_) {
    sent_destroyed_notification_ = true;

    NotifyWillBeDestroyed(this);
    content::NotificationService::current()->Notify(
        chrome::NOTIFICATION_PROFILE_DESTROYED,
        content::Source<Profile>(this),
        content::NotificationService::NoDetails());
  }
}

bool ProfileCompare::operator()(Profile* a, Profile* b) const {
  DCHECK(a && b);
  if (a->IsSameProfile(b))
    return false;
  return a->GetOriginalProfile() < b->GetOriginalProfile();
}

double Profile::GetDefaultZoomLevelForProfile() {
  return GetDefaultStoragePartition(this)
      ->GetHostZoomMap()
      ->GetDefaultZoomLevel();
}

bool Profile::IsGuestSession() const {
  return false;
}

bool Profile::IsNewProfile() {
  // The profile has been shut down if the prefs were loaded from disk, unless
  // first-run autoimport wrote them and reloaded the pref service.
  // TODO(crbug.com/660346): revisit this when crbug.com/22142 (unifying the
  // profile import code) is fixed.
  return GetOriginalProfile()->GetPrefs()->GetInitializationStatus() ==
      PrefService::INITIALIZATION_STATUS_CREATED_NEW_PREF_STORE;
}

