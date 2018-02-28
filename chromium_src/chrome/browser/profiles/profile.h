// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This class gathers state related to a single user profile.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_H_
#define CHROME_BROWSER_PROFILES_PROFILE_H_

#include <string>

#include "atom/browser/atom_browser_context.h"
#include "base/logging.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"

namespace autofill {
class AutofillWebDataService;
}

#if defined(OS_WIN)
class PasswordWebDataService;
#endif

namespace base {
class DictionaryValue;
}

namespace content {
class WebUI;
}

namespace net {
class URLRequestContextGetter;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class ChromeZoomLevelPrefs;
class PrefChangeRegistrar;

// Instead of adding more members to Profile, consider creating a
// KeyedService. See
// http://dev.chromium.org/developers/design-documents/profile-architecture
class Profile : public atom::AtomBrowserContext {
 public:
  enum CreateStatus {
    // Profile services were not created due to a local error (e.g., disk full).
    CREATE_STATUS_LOCAL_FAIL,
    // Profile services were not created due to a remote error (e.g., network
    // down during limited-user registration).
    CREATE_STATUS_REMOTE_FAIL,
    // Profile created but before initializing extensions and promo resources.
    CREATE_STATUS_CREATED,
    // Profile is created, extensions and promo resources are initialized.
    CREATE_STATUS_INITIALIZED,
    // Profile creation (supervised-user registration, generally) was canceled
    // by the user.
    CREATE_STATUS_CANCELED,
    MAX_CREATE_STATUS  // For histogram display.
  };

  enum CreateMode {
    CREATE_MODE_SYNCHRONOUS,
    CREATE_MODE_ASYNCHRONOUS
  };

  enum ExitType {
    // A normal shutdown. The user clicked exit/closed last window of the
    // profile.
    EXIT_NORMAL,

    // The exit was the result of the system shutting down.
    EXIT_SESSION_ENDED,

    EXIT_CRASHED,
  };

  enum ProfileType {
    REGULAR_PROFILE,  // Login user's normal profile
    INCOGNITO_PROFILE,  // Login user's off-the-record profile
    GUEST_PROFILE,  // Guest session's profile
  };

  class Delegate {
   public:
    virtual ~Delegate();

    // Called when creation of the profile is finished.
    virtual void OnProfileCreated(Profile* profile,
                                  bool success,
                                  bool is_new_profile) = 0;
  };

  Profile(const std::string& partition, bool in_memory,
                     const base::DictionaryValue& options);
  ~Profile() override;

  static Profile* FromWebUI(content::WebUI* web_ui);
  // Returns the profile corresponding to the given browser context.
  static Profile* FromBrowserContext(content::BrowserContext* browser_context);

  // Return the incognito version of this profile. The returned pointer
  // is owned by the receiving profile. If the receiving profile is off the
  // record, the same profile is returned.
  //
  // WARNING: This will create the OffTheRecord profile if it doesn't already
  // exist. If this isn't what you want, you need to check
  // HasOffTheRecordProfile() first.
  virtual Profile* GetOffTheRecordProfile() = 0;

  // Returns the main request context.
  virtual net::URLRequestContextGetter* GetRequestContext() = 0;

  // True if an incognito profile exists.
  virtual bool HasOffTheRecordProfile() = 0;

  // Return the original "recording" profile. This method returns this if the
  // profile is not incognito.
  virtual Profile* GetOriginalProfile() = 0;
 
  bool IsLegacySupervised() {return false;}

  // Return the original "recording" profile. This method returns this if the
  // profile is not incognito.
  virtual const Profile* GetOriginalProfile() const = 0;

  // Retrieves a pointer to the PrefService that manages the default zoom
  // level and the per-host zoom levels for this user profile.
  // TODO(wjmaclean): Remove this when HostZoomMap migrates to StoragePartition.
  virtual ChromeZoomLevelPrefs* GetZoomLevelPrefs();

  virtual base::FilePath last_selected_directory() = 0;
  virtual void set_last_selected_directory(const base::FilePath& path) = 0;

  // Return whether 2 profiles are the same. 2 profiles are the same if they
  // represent the same profile. This can happen if there is pointer equality
  // or if one profile is the incognito version of another profile (or vice
  // versa).
  virtual bool IsSameProfile(Profile* profile) = 0;

  // Send NOTIFICATION_PROFILE_DESTROYED for this Profile, if it has not
  // already been sent. It is necessary because most Profiles are destroyed by
  // ProfileDestroyer, but in tests, some are not.
  void MaybeSendDestroyedNotification();

  // Convenience method to retrieve the default zoom level for the default
  // storage partition.
  double GetDefaultZoomLevelForProfile();

  virtual sync_preferences::PrefServiceSyncable* GetPrefs() = 0;
  virtual const sync_preferences::PrefServiceSyncable* GetPrefs() const = 0;

  virtual user_prefs::PrefRegistrySyncable* pref_registry() const = 0;

  virtual void AddOverlayPref(const std::string name) = 0;

  virtual scoped_refptr<autofill::AutofillWebDataService>
    GetAutofillWebdataService() = 0;

#if defined(OS_WIN)
  virtual scoped_refptr<PasswordWebDataService>
    GetPasswordWebdataService() = 0;
#endif

  virtual PrefChangeRegistrar* user_prefs_change_registrar() const = 0;

  virtual bool IsGuestSession() const;

  bool IsNewProfile();
  virtual void SetExitType(ExitType exit_type) = 0;

  // Returns sequenced task runner where browser context dependent I/O
  // operations should be performed.
  virtual scoped_refptr<base::SequencedTaskRunner> GetIOTaskRunner() = 0;

 private:
  // bool restored_last_session_;

  // Used to prevent the notification that this Profile is destroyed from
  // being sent twice.
  bool sent_destroyed_notification_;

  // Accessibility events will only be propagated when the pause
  // level is zero.  PauseAccessibilityEvents and ResumeAccessibilityEvents
  // increment and decrement the level, respectively, rather than set it to
  // true or false, so that calls can be nested.
  // int accessibility_pause_level_;

  // A non-browsing profile not associated to a user. Sample use: User-Manager.
  // bool is_system_profile_;

  DISALLOW_COPY_AND_ASSIGN(Profile);
};

// The comparator for profile pointers as key in a map.
struct ProfileCompare {
  bool operator()(Profile* a, Profile* b) const;
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_H_
