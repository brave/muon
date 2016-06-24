// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include <string>
#include <vector>
#include "base/memory/weak_ptr.h"
#include "brightray/browser/browser_context.h"

class PrefChangeRegistrar;

namespace syncable_prefs {
class PrefServiceSyncable;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace atom {

class AtomDownloadManagerDelegate;
class AtomCertVerifier;
class AtomNetworkDelegate;
class AtomPermissionManager;
class WebViewManager;

class AtomBrowserContext : public brightray::BrowserContext {
 public:
  AtomBrowserContext(const std::string& partition, bool in_memory);
  ~AtomBrowserContext() override;

  using brightray::BrowserContext::GetPath;

  // brightray::URLRequestContextGetter::Delegate:
  net::NetworkDelegate* CreateNetworkDelegate() override;
  std::string GetUserAgent() override;
  std::unique_ptr<net::URLRequestJobFactory> CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* protocol_handlers) override;
  net::HttpCache::BackendFactory* CreateHttpCacheBackendFactory(
      const base::FilePath& base_path) override;
  std::unique_ptr<net::CertVerifier> CreateCertVerifier() override;
  net::SSLConfigService* CreateSSLConfigService() override;

  // content::BrowserContext:
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  content::PermissionManager* GetPermissionManager() override;

  // brightray::BrowserContext:
  void RegisterPrefs(PrefRegistrySimple* pref_registry) override;

  AtomCertVerifier* cert_verifier() const { return cert_verifier_; }
  AtomNetworkDelegate* network_delegate() const { return network_delegate_; }

  AtomBrowserContext* original_context();
  AtomBrowserContext* otr_context();

#if defined(ENABLE_EXTENSIONS)
  user_prefs::PrefRegistrySyncable* pref_registry() const {
    return pref_registry_.get(); }

  syncable_prefs::PrefServiceSyncable* user_prefs() const {
    return user_prefs_.get(); }

  PrefChangeRegistrar* user_prefs_change_registrar() const {
    return user_prefs_registrar_.get(); }

  const std::string& partition() const { return partition_; }

  void AddOverlayPref(const std::string name) {
    overlay_pref_names_.push_back(name.c_str()); }
#endif

 private:
#if defined(ENABLE_EXTENSIONS)
  void RegisterUserPrefs();
  void OnPrefsLoaded(bool success);
  scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry_;
  std::unique_ptr<syncable_prefs::PrefServiceSyncable> user_prefs_;
  std::unique_ptr<PrefChangeRegistrar> user_prefs_registrar_;
  std::vector<const char*> overlay_pref_names_;
#endif

  std::unique_ptr<AtomDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<WebViewManager> guest_manager_;
  std::unique_ptr<AtomPermissionManager> permission_manager_;

  // Managed by brightray::BrowserContext.
  AtomCertVerifier* cert_verifier_;
  AtomNetworkDelegate* network_delegate_;

  scoped_refptr<brightray::BrowserContext> original_context_;
  scoped_refptr<brightray::BrowserContext> otr_context_;
  const std::string partition_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
