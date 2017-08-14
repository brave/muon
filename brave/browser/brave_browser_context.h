// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_CONTEXT_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_CONTEXT_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/browser/atom_browser_context.h"
#include "content/public/browser/host_zoom_map.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/zoom/chrome_zoom_level_prefs.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#if defined(OS_WIN)
#include "components/password_manager/core/browser/webdata/password_web_data_service_win.h"
#endif
#include "components/prefs/overlay_user_pref_store.h"
#include "components/webdata/common/web_database_service.h"

class PrefChangeRegistrar;

namespace sync_preferences {
class PrefServiceSyncable;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {

class BravePermissionManager;

class BraveBrowserContext : public Profile {
 public:
  BraveBrowserContext(const std::string& partition,
                      bool in_memory,
                      const base::DictionaryValue& options,
                      scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~BraveBrowserContext() override;

  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;

  static atom::AtomBrowserContext* FromPartition(
    const std::string& partition, const base::DictionaryValue& options);

  static BraveBrowserContext*
      FromBrowserContext(content::BrowserContext* browser_context);

  // brightray::URLRequestContextGetter::Delegate:
  std::unique_ptr<net::URLRequestJobFactory> CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* protocol_handlers) override;

  void CreateProfilePrefs(scoped_refptr<base::SequencedTaskRunner> task_runner);

  ChromeZoomLevelPrefs* GetZoomLevelPrefs() override;

  bool HasParentContext();

  // content::BrowserContext:
  content::PermissionManager* GetPermissionManager() override;
  content::ResourceContext* GetResourceContext() override;
  net::NetworkDelegate* CreateNetworkDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;

  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate()
      override {
    return nullptr;
  }

  // atom::AtomBrowserContext
  atom::AtomNetworkDelegate* network_delegate() override;

  // Profile
  net::URLRequestContextGetter* GetRequestContext() override;

  BraveBrowserContext* original_context();
  BraveBrowserContext* otr_context();

  Profile* GetOffTheRecordProfile() override;
  bool HasOffTheRecordProfile() override;
  Profile* GetOriginalProfile() override;
  bool IsSameProfile(Profile* profile) override;

  user_prefs::PrefRegistrySyncable* pref_registry() const override {
    return pref_registry_.get(); }

  sync_preferences::PrefServiceSyncable* user_prefs() const {
    return user_prefs_.get(); }

  sync_preferences::PrefServiceSyncable* GetPrefs() {
    return user_prefs_.get(); }

  const sync_preferences::PrefServiceSyncable* GetPrefs() const override {
    return static_cast<const Profile*>(this)->GetPrefs();
  }

  PrefChangeRegistrar* user_prefs_change_registrar() const override {
    return user_prefs_registrar_.get(); }

  const std::string& partition() const { return partition_; }
  std::string partition_with_prefix();
  base::WaitableEvent* ready() { return ready_.get(); }

  void AddOverlayPref(const std::string name) override {
    overlay_pref_names_.push_back(name.c_str()); }

  scoped_refptr<autofill::AutofillWebDataService>
    GetAutofillWebdataService() override;

#if defined(OS_WIN)
  scoped_refptr<PasswordWebDataService>
    GetPasswordWebdataService() override;
#endif

  base::FilePath GetPath() const override;

  DevToolsNetworkControllerHandle*
  GetDevToolsNetworkControllerHandle() override {
    return network_controller_handle();
  }

 private:
  void OnPrefsLoaded(bool success);
  void TrackZoomLevelsFromParent();
  void OnParentZoomLevelChanged(
      const content::HostZoomMap::ZoomLevelChange& change);
  void UpdateDefaultZoomLevel();

  scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry_;
  std::unique_ptr<sync_preferences::PrefServiceSyncable> user_prefs_;
  std::unique_ptr<PrefChangeRegistrar> user_prefs_registrar_;
  std::vector<const char*> overlay_pref_names_;

  std::unique_ptr<content::HostZoomMap::Subscription> track_zoom_subscription_;
    std::unique_ptr<ChromeZoomLevelPrefs::DefaultZoomLevelSubscription>
        parent_default_zoom_level_subscription_;

  std::unique_ptr<BravePermissionManager> permission_manager_;

  bool has_parent_;
  BraveBrowserContext* original_context_;
  BraveBrowserContext* otr_context_;
  const std::string partition_;
  std::unique_ptr<base::WaitableEvent> ready_;

  scoped_refptr<autofill::AutofillWebDataService> autofill_data_;
#if defined(OS_WIN)
  scoped_refptr<PasswordWebDataService> password_data_;
#endif
  scoped_refptr<WebDatabaseService> web_database_;
  std::unique_ptr<ProtocolHandlerRegistry::JobInterceptorFactory>
      protocol_handler_interceptor_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserContext);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_CONTEXT_H_
