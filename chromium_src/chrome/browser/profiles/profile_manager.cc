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
#include "base/metrics/user_metrics.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "atom/browser/atom_browser_context.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile_info_cache.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/browser/ui/browser_list.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/common/content_switches.h"
#include "extensions/features/features.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_system.h"
#endif

using base::UserMetricsAction;
using content::BrowserThread;

ProfileManager::ProfileManager(const base::FilePath& user_data_dir)
    : user_data_dir_(user_data_dir),
      browser_list_observer_(this) {
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

void ProfileManager::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {}

void ProfileManager::OnProfileCreated(Profile* profile,
                                      bool success,
                                      bool is_new_profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!profile->IsOffTheRecord() &&
      !GetProfileByPath(profile->GetPath())) {
    AddProfile(profile);
  }

  DoFinalInit(profile, false);
}

void ProfileManager::DoFinalInit(Profile* profile, bool go_off_the_record) {
  TRACE_EVENT0("browser", "ProfileManager::DoFinalInit");

  DoFinalInitForServices(profile, go_off_the_record);
  DoFinalInitLogging(profile);

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_PROFILE_ADDED,
      content::Source<Profile>(profile),
      content::NotificationService::NoDetails());
}

void ProfileManager::DoFinalInitForServices(Profile* profile,
                                            bool go_off_the_record) {
  TRACE_EVENT0("browser", "ProfileManager::DoFinalInitForServices");

#if BUILDFLAG(ENABLE_EXTENSIONS)
  bool extensions_enabled = !go_off_the_record;

  extensions::ExtensionSystem::Get(profile)->InitForRegularProfile(
      extensions_enabled);
#endif
}

void ProfileManager::DoFinalInitLogging(Profile* profile) {}

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
  Profile* profile = GetProfileByPath(profile_dir);
  if (profile)
    return profile;
  return CreateAndInitializeProfile(profile_dir);
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

Profile* ProfileManager::CreateProfileAsyncHelper(const base::FilePath& path,
                                                  Delegate* delegate) {
  NOTIMPLEMENTED();
  return nullptr;
}


Profile* ProfileManager::CreateProfileHelper(const base::FilePath& path) {
  TRACE_EVENT0("browser", "ProfileManager::CreateProfileHelper");
  SCOPED_UMA_HISTOGRAM_TIMER("Profile.CreateProfileHelperTime");

  std::string partition = "";
  if (path != user_data_dir()) {
    partition = path.BaseName().AsUTF8Unsafe();
  }

  return static_cast<Profile*>(
      atom::AtomBrowserContext::From(partition, false));
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

Profile* ProfileManager::CreateAndInitializeProfile(
    const base::FilePath& profile_dir) {
  TRACE_EVENT0("browser", "ProfileManager::CreateAndInitializeProfile");
  SCOPED_UMA_HISTOGRAM_LONG_TIMER("Profile.CreateAndInitializeProfile");
  // CHECK that we are not trying to load the same profile twice, to prevent
  // profile corruption. Note that this check also covers the case when we have
  // already started loading the profile but it is not fully initialized yet,
  // which would make Bad Things happen if we returned it.
  CHECK(!GetProfileByPathInternal(profile_dir));
  Profile* profile = CreateProfileHelper(profile_dir);
  // BraveBrowserContext currently initializes the profile
  // if (profile) {
  //   bool result = AddProfile(profile);
  //   DCHECK(result);
  // }
  return profile;
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

// static
void ProfileManager::NukeDeletedProfilesFromDisk() {
  // TODO(bridiver)
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

ProfileManager::BrowserListObserver::BrowserListObserver(
    ProfileManager* manager)
    : profile_manager_(manager) {
  CHECK(profile_manager_);
  BrowserList::AddObserver(this);
}

ProfileManager::BrowserListObserver::~BrowserListObserver() {
  BrowserList::RemoveObserver(this);
}

void ProfileManager::BrowserListObserver::OnBrowserAdded(
    Browser* browser) {}

void ProfileManager::BrowserListObserver::OnBrowserRemoved(
    Browser* browser) {}

void ProfileManager::BrowserListObserver::OnBrowserSetLastActive(
    Browser* browser) {}
