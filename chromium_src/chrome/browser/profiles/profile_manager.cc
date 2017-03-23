// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile_manager.h"

#include <stdint.h>

#include <map>
#include <set>
#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/deferred_sequenced_task_runner.h"
#include "base/feature_list.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
// #include "chrome/browser/bookmarks/bookmark_model_factory.h"
// #include "chrome/browser/bookmarks/startup_task_runner_service_factory.h"
#include "chrome/browser/browser_process.h"
// #include "chrome/browser/chrome_notification_types.h"
// #include "chrome/browser/content_settings/host_content_settings_map_factory.h"
// #include "chrome/browser/download/download_service.h"
// #include "chrome/browser/download/download_service_factory.h"
// #include "chrome/browser/invalidation/profile_invalidation_provider_factory.h"
// #include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings.h"
// #include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings_factory.h"
// #include "chrome/browser/password_manager/password_manager_setting_migrator_service_factory.h"
// #include "chrome/browser/password_manager/password_store_factory.h"
// #include "chrome/browser/prefs/incognito_mode_prefs.h"
// #include "chrome/browser/profiles/bookmark_model_loaded_observer.h"
// #include "chrome/browser/profiles/profile_attributes_entry.h"
// #include "chrome/browser/profiles/profile_attributes_storage.h"
// #include "chrome/browser/profiles/profile_avatar_icon_util.h"
// #include "chrome/browser/profiles/profile_destroyer.h"
// #include "chrome/browser/profiles/profile_info_cache.h"
// #include "chrome/browser/profiles/profile_metrics.h"
#include "chrome/browser/profiles/profiles_state.h"
// #include "chrome/browser/sessions/session_service_factory.h"
// #include "chrome/browser/signin/account_fetcher_service_factory.h"
// #include "chrome/browser/signin/account_reconcilor_factory.h"
// #include "chrome/browser/signin/account_tracker_service_factory.h"
// #include "chrome/browser/signin/cross_device_promo.h"
// #include "chrome/browser/signin/cross_device_promo_factory.h"
// #include "chrome/browser/signin/gaia_cookie_manager_service_factory.h"
// #include "chrome/browser/signin/signin_manager_factory.h"
// #include "chrome/browser/sync/profile_sync_service_factory.h"
// #include "chrome/browser/ui/browser.h"
// #include "chrome/browser/ui/browser_list.h"
// #include "chrome/browser/ui/sync/sync_promo_ui.h"
// #include "chrome/common/chrome_constants.h"
// #include "chrome/common/chrome_paths_internal.h"
// #include "chrome/common/chrome_switches.h"
// #include "chrome/common/logging_chrome.h"
// #include "chrome/common/pref_names.h"
// #include "chrome/common/url_constants.h"
// #include "chrome/grit/generated_resources.h"
// #include "components/bookmarks/browser/bookmark_model.h"
// #include "components/bookmarks/browser/startup_task_runner_service.h"
// #include "components/bookmarks/common/bookmark_pref_names.h"
// #include "components/browser_sync/browser/profile_sync_service.h"
// #include "components/content_settings/core/browser/host_content_settings_map.h"
// #include "components/invalidation/impl/profile_invalidation_provider.h"
// #include "components/invalidation/public/invalidation_service.h"
// #include "components/password_manager/core/browser/password_store.h"
// #include "components/password_manager/sync/browser/password_manager_setting_migrator_service.h"
// #include "components/prefs/pref_service.h"
// #include "components/prefs/scoped_user_pref_update.h"
// #include "components/search_engines/default_search_manager.h"
// #include "components/signin/core/browser/account_fetcher_service.h"
// #include "components/signin/core/browser/account_tracker_service.h"
// #include "components/signin/core/browser/gaia_cookie_manager_service.h"
// #include "components/signin/core/browser/signin_manager.h"
// #include "components/signin/core/common/profile_management_switches.h"
// #include "components/signin/core/common/signin_pref_names.h"
// #include "components/sync/base/stop_source.h"
#include "content/public/browser/browser_thread.h"
// #include "content/public/browser/notification_service.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/common/content_switches.h"
// #include "extensions/features/features.h"
// #include "net/http/http_transaction_factory.h"
// #include "net/url_request/url_request_context.h"
// #include "net/url_request/url_request_context_getter.h"
// #include "net/url_request/url_request_job.h"
// #include "ui/base/l10n/l10n_util.h"

// #if BUILDFLAG(ENABLE_EXTENSIONS)
// #include "chrome/browser/extensions/extension_service.h"
// #include "extensions/browser/extension_registry.h"
// #include "extensions/browser/extension_system.h"
// #include "extensions/common/extension_set.h"
// #include "extensions/common/manifest.h"
// #endif

// #if defined(ENABLE_SUPERVISED_USERS)
// #include "chrome/browser/supervised_user/child_accounts/child_account_service.h"
// #include "chrome/browser/supervised_user/child_accounts/child_account_service_factory.h"
// #include "chrome/browser/supervised_user/supervised_user_service.h"
// #include "chrome/browser/supervised_user/supervised_user_service_factory.h"
// #endif

// #if defined(OS_ANDROID)
// #include "chrome/browser/ntp_snippets/content_suggestions_service_factory.h"
// #endif

// #if defined(OS_CHROMEOS)
// #include "chrome/browser/browser_process_platform_part_chromeos.h"
// #include "chrome/browser/chromeos/profiles/profile_helper.h"
// #include "chromeos/chromeos_switches.h"
// #include "chromeos/dbus/cryptohome_client.h"
// #include "chromeos/dbus/dbus_thread_manager.h"
// #include "components/user_manager/user.h"
// #include "components/user_manager/user_manager.h"
// #endif

// #if !defined(OS_ANDROID) && !defined(OS_IOS) && !defined(OS_CHROMEOS)
// #include "chrome/browser/profiles/profile_statistics.h"
// #include "chrome/browser/profiles/profile_statistics_factory.h"
// #endif

using base::UserMetricsAction;
using content::BrowserThread;

ProfileManager::ProfileManager(const base::FilePath& user_data_dir)
    : user_data_dir_(user_data_dir) {
}

ProfileManager::~ProfileManager() {
  // destroy child profiles
  ProfilesInfoMap::iterator itr = profiles_info_.begin();
  while (itr != profiles_info_.end()) {
    if (itr->second->profile->GetOriginalProfile() != itr->second->profile.get()) {
      delete itr->second.release();
      profiles_info_.erase(itr++);
    } else {
      ++itr;
    }
  }

  // destroy parent profiles
  itr = profiles_info_.begin();
  while (itr != profiles_info_.end()) {
    // delete otr profile
    if (itr->second->profile->HasOffTheRecordProfile())
      delete itr->second->profile->GetOffTheRecordProfile();

    delete itr->second.release();
    profiles_info_.erase(itr++);
  }

  profiles_info_.clear();
}

// static
Profile* ProfileManager::GetLastUsedProfile() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  return profile_manager->GetLastUsedProfile(profile_manager->user_data_dir_);
}

// static
Profile* ProfileManager::GetLastUsedProfileAllowedByPolicy() {
  return GetLastUsedProfile();
}

// static
bool ProfileManager::IncognitoModeForced(Profile* profile) {
  return false;
}

// static
std::vector<Profile*> ProfileManager::GetLastOpenedProfiles() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  return profile_manager->GetLastOpenedProfiles(
      profile_manager->user_data_dir_);
}

// static
Profile* ProfileManager::GetPrimaryUserProfile() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  return profile_manager->GetActiveUserOrOffTheRecordProfileFromPath(
      profile_manager->user_data_dir());
}

// static
Profile* ProfileManager::GetActiveUserProfile() {
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  Profile* profile =
      profile_manager->GetActiveUserOrOffTheRecordProfileFromPath(
          profile_manager->user_data_dir());
  // |profile| could be null if the user doesn't have a profile yet and the path
  // is on a read-only volume (preventing Chrome from making a new one).
  // However, most callers of this function immediately dereference the result
  // which would lead to crashes in a variety of call sites. Assert here to
  // figure out how common this is. http://crbug.com/383019
  CHECK(profile) << profile_manager->user_data_dir().AsUTF8Unsafe();
  return profile;
}

Profile* ProfileManager::GetProfile(const base::FilePath& profile_dir) {
  TRACE_EVENT0("browser", "ProfileManager::GetProfile");

  // If the profile is already loaded (e.g., chrome.exe launched twice), just
  // return it.
  return GetProfileByPath(profile_dir);
}

bool ProfileManager::IsValidProfile(const void* profile) {
  for (ProfilesInfoMap::iterator iter = profiles_info_.begin();
       iter != profiles_info_.end(); ++iter) {
    if (iter->second->created) {
      Profile* candidate = iter->second->profile.get();
      if (candidate == profile ||
          (candidate->HasOffTheRecordProfile() &&
           candidate->GetOffTheRecordProfile() == profile)) {
        return true;
      }
    }
  }
  return false;
}

base::FilePath ProfileManager::GetInitialProfileDir() {
  base::FilePath relative_profile_dir;
  // TODO(mirandac): should not automatically be default profile.
  return relative_profile_dir.AppendASCII("");
}

Profile* ProfileManager::GetLastUsedProfile(
    const base::FilePath& user_data_dir) {
  return GetProfile(GetLastUsedProfileDir(user_data_dir));
}

base::FilePath ProfileManager::GetLastUsedProfileDir(
    const base::FilePath& user_data_dir) {
  return user_data_dir.AppendASCII(GetLastUsedProfileName());
}

std::string ProfileManager::GetLastUsedProfileName() {
  return "";
}

std::vector<Profile*> ProfileManager::GetLastOpenedProfiles(
    const base::FilePath& user_data_dir) {
  std::vector<Profile*> to_return;
  to_return.push_back(GetLastUsedProfile(user_data_dir));
  return to_return;
}

std::vector<Profile*> ProfileManager::GetLoadedProfiles() const {
  std::vector<Profile*> profiles;
  for (ProfilesInfoMap::const_iterator iter = profiles_info_.begin();
       iter != profiles_info_.end(); ++iter) {
    if (iter->second->created)
      profiles.push_back(iter->second->profile.get());
  }
  return profiles;
}

Profile* ProfileManager::GetProfileByPathInternal(
    const base::FilePath& path) const {
  TRACE_EVENT0("browser", "ProfileManager::GetProfileByPathInternal");
  ProfileInfo* profile_info = GetProfileInfoByPath(path);
  return profile_info ? profile_info->profile.get() : nullptr;
}

Profile* ProfileManager::GetProfileByPath(const base::FilePath& path) const {
  TRACE_EVENT0("browser", "ProfileManager::GetProfileByPath");
  ProfileInfo* profile_info = GetProfileInfoByPath(path);
  return (profile_info && profile_info->created) ? profile_info->profile.get()
                                                 : nullptr;
}

Profile* ProfileManager::GetActiveUserOrOffTheRecordProfileFromPath(
    const base::FilePath& user_data_dir) {
  base::FilePath default_profile_dir(user_data_dir);
  default_profile_dir = default_profile_dir.Append(GetInitialProfileDir());
  return GetProfile(default_profile_dir);
}

bool ProfileManager::AddProfile(Profile* profile) {
  DCHECK(profile);

  if (GetProfileByPathInternal(profile->GetPath())) {
    NOTREACHED() << "Attempted to add profile with the same path (" <<
                    profile->GetPath().value() <<
                    ") as an already-loaded profile.";
    return false;
  }

  RegisterProfile(profile, true);
  return true;
}

ProfileManager::ProfileInfo* ProfileManager::RegisterProfile(
    Profile* profile,
    bool created) {
  TRACE_EVENT0("browser", "ProfileManager::RegisterProfile");
  ProfileInfo* info = new ProfileInfo(profile, created);
  profiles_info_.insert(
      std::make_pair(profile->GetPath(), linked_ptr<ProfileInfo>(info)));
  return info;
}

ProfileManager::ProfileInfo* ProfileManager::GetProfileInfoByPath(
    const base::FilePath& path) const {
  ProfilesInfoMap::const_iterator iter = profiles_info_.find(path);
  return (iter == profiles_info_.end()) ? NULL : iter->second.get();
}

ProfileManager::ProfileInfo::ProfileInfo(
    Profile* profile,
    bool created)
    : profile(profile),
      created(created) {
}

ProfileManager::ProfileInfo::~ProfileInfo() {
  delete profile.release();
}
