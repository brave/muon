// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class keeps track of the currently-active profiles in the runtime.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_
#define CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_

#include <stddef.h>

#include <list>
#include <memory>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_checker.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"

class ProfileManager {
 public:
  explicit ProfileManager(const base::FilePath& user_data_dir);
  ~ProfileManager();

  // Same as instance method but provides the default user_data_dir as well.
  // If the Profile is going to be used to open a new window then consider using
  // GetLastUsedProfileAllowedByPolicy() instead.
  static Profile* GetLastUsedProfile();

  // Same as GetLastUsedProfile() but returns the incognito Profile if
  // incognito mode is forced. This should be used if the last used Profile
  // will be used to open new browser windows.
  static Profile* GetLastUsedProfileAllowedByPolicy();

  // Helper function that returns true if incognito mode is forced for |profile|
  // (normal mode is not available for browsing).
  static bool IncognitoModeForced(Profile* profile);

  // Same as instance method but provides the default user_data_dir as well.
  static std::vector<Profile*> GetLastOpenedProfiles();

  // Get the profile for the user which created the current session.
  // Note that in case of a guest account this will return a 'suitable' profile.
  // This function is temporary and will soon be moved to ash. As such avoid
  // using it at all cost.
  // TODO(skuhne): Move into ash's new user management function.
  static Profile* GetPrimaryUserProfile();

  // Get the profile for the currently active user.
  // Note that in case of a guest account this will return a 'suitable' profile.
  // This function is temporary and will soon be moved to ash. As such avoid
  // using it at all cost.
  // TODO(skuhne): Move into ash's new user management function.
  static Profile* GetActiveUserProfile();

  // Returns a profile for a specific profile directory within the user data
  // dir. This will return an existing profile it had already been created,
  // otherwise it will create and manage it.
  // Because this method might synchronously create a new profile, it should
  // only be called for the initial profile or in tests, where blocking is
  // acceptable. Returns null if creation of the new profile fails.
  // TODO(bauerb): Migrate calls from other code to GetProfileByPath(), then
  // make this method private.
  Profile* GetProfile(const base::FilePath& profile_dir);

  // Returns true if the profile pointer is known to point to an existing
  // profile.
  bool IsValidProfile(const void* profile);

  // Returns the directory where the first created profile is stored,
  // relative to the user data directory currently in use.
  base::FilePath GetInitialProfileDir();

  // Get the Profile last used (the Profile to which owns the most recently
  // focused window) with this Chrome build. If no signed profile has been
  // stored in Local State, hand back the Default profile.
  Profile* GetLastUsedProfile(const base::FilePath& user_data_dir);

  // Get the path of the last used profile, or if that's undefined, the default
  // profile.
  base::FilePath GetLastUsedProfileDir(const base::FilePath& user_data_dir);

  // Get the name of the last used profile, or if that's undefined, the default
  // profile.
  std::string GetLastUsedProfileName();

  // Get the Profiles which are currently open, i.e. have open browsers or were
  // open the last time Chrome was running. Profiles that fail to initialize are
  // skipped. The Profiles appear in the order they were opened. The last used
  // profile will be on the list if it is initialized successfully, but its
  // index on the list will depend on when it was opened (it is not necessarily
  // the last one).
  std::vector<Profile*> GetLastOpenedProfiles(
      const base::FilePath& user_data_dir);

  // Returns created and fully initialized profiles. Note, profiles order is NOT
  // guaranteed to be related with the creation order.
  std::vector<Profile*> GetLoadedProfiles() const;

  // If a profile with the given path is currently managed by this object and
  // fully initialized, return a pointer to the corresponding Profile object;
  // otherwise return null.
  Profile* GetProfileByPath(const base::FilePath& path) const;

  bool AddProfile(Profile* profile);

  Profile* CreateAndInitializeProfile(const base::FilePath& profile_dir);

  const base::FilePath& user_data_dir() const { return user_data_dir_; }

 protected:
  virtual Profile* CreateProfileHelper(const base::FilePath& path);

 private:
  // This struct contains information about profiles which are being loaded or
  // were loaded.
  struct ProfileInfo {
    ProfileInfo(Profile* profile, bool created);

    ~ProfileInfo();

    std::unique_ptr<Profile> profile;
    // Whether profile has been fully loaded (created and initialized).
    bool created;
   private:
    DISALLOW_COPY_AND_ASSIGN(ProfileInfo);
  };

  // Returns the profile of the active user and / or the off the record profile
  // if needed. This adds the profile to the ProfileManager if it doesn't
  // already exist. The method will return NULL if the profile doesn't exist
  // and we can't create it.
  // The profile used can be overridden by using --login-profile on cros.
  Profile* GetActiveUserOrOffTheRecordProfileFromPath(
      const base::FilePath& user_data_dir);

  // Registers profile with given info. Returns pointer to created ProfileInfo
  // entry.
  ProfileInfo* RegisterProfile(Profile* profile, bool created);

  // Returns ProfileInfo associated with given |path|, registered earlier with
  // RegisterProfile.
  ProfileInfo* GetProfileInfoByPath(const base::FilePath& path) const;

  // Returns a registered profile. In contrast to GetProfileByPath(), this will
  // also return a profile that is not fully initialized yet, so this method
  // should be used carefully.
  Profile* GetProfileByPathInternal(const base::FilePath& path) const;

  // The path to the user data directory (DIR_USER_DATA).
  const base::FilePath user_data_dir_;

  // Maps profile path to ProfileInfo (if profile has been created). Use
  // RegisterProfile() to add into this map. This map owns all loaded profile
  // objects in a running instance of Chrome.
  typedef std::map<base::FilePath, linked_ptr<ProfileInfo> > ProfilesInfoMap;
  ProfilesInfoMap profiles_info_;

  // TODO(chrome/browser/profiles/OWNERS): Usage of this in profile_manager.cc
  // should likely be turned into DCHECK_CURRENTLY_ON(BrowserThread::UI) for
  // consistency with surrounding code in the same file but that wasn't trivial
  // enough to do as part of the mass refactor CL which introduced
  // |thread_checker_|, ref. https://codereview.chromium.org/2907253003/#msg37.
  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(ProfileManager);
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_
