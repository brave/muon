// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_IO_THREAD_H_
#define CHROME_BROWSER_IO_THREAD_H_

#include "base/macros.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "components/prefs/pref_member.h"
#include "components/ssl_config/ssl_config_service_manager.h"
#include "content/public/browser/browser_thread_delegate.h"
#include "content/public/network/network_service.h"

class PrefProxyConfigTracker;
class PrefService;

namespace certificate_transparency {
class TreeStateTracker;
}

namespace content {
class URLRequestContextBuilderMojo;
}

namespace net {
class CTLogVerifier;
class HostResolver;
class HttpAuthHandlerFactory;
class HttpAuthPreferences;
class ProxyConfigService;
class URLRequestContext;
class URLRequestContextGetter;

namespace ct {
class STHObserver;
}
} // namespace net

namespace net_log {
class ChromeNetLog;
}

class IOThread : public content::BrowserThreadDelegate {
  public:

  struct Globals {
    class SystemRequestContextLeakChecker {
     public:
      explicit SystemRequestContextLeakChecker(Globals* globals);
      ~SystemRequestContextLeakChecker();

     private:
      Globals* const globals_;
    };

    Globals();
    ~Globals();

    // In-process NetworkService for use in URLRequestContext configuration when
    // the network service created through the ServiceManager is disabled. See
    // SystemNetworkContextManager's header comment for more details
    std::unique_ptr<content::NetworkService> network_service;

    std::vector<scoped_refptr<const net::CTLogVerifier>> ct_logs;
    std::unique_ptr<net::HttpAuthPreferences> http_auth_preferences;
    std::unique_ptr<content::mojom::NetworkContext> system_network_context;
    net::URLRequestContext* system_request_context;
    SystemRequestContextLeakChecker system_request_context_leak_checker;
  };


  // |net_log| must either outlive the IOThread or be NULL.
  IOThread(PrefService* local_state,
           net_log::ChromeNetLog* net_log,
	   SystemNetworkContextManager* system_network_context_manager);
  ~IOThread();

  // BrowserThreadDelegate implementation, runs on the IO thread.
  // This handles initialization and destruction of state that must
  // live on the IO thread.
  void Init() override;
  void CleanUp() override;

  std::unique_ptr<net::HttpAuthHandlerFactory> CreateDefaultAuthHandlerFactory(
      net::HostResolver* host_resolver);

  // Returns an SSLConfigService instance.
  net::SSLConfigService* GetSSLConfigService();

  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Can only be called on the IO thread.
  Globals* globals();

  // Registers the |observer| for new STH notifications.
  void RegisterSTHObserver(net::ct::STHObserver* observer);

  // Un-registers the |observer|.
  void UnregisterSTHObserver(net::ct::STHObserver* observer);

#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  // No PrefMember for the GSSAPI library name, since changing it after startup
  // requires unloading the existing GSSAPI library, which could cause all sorts
  // of problems for, for example, active Negotiate transactions.
  std::string gssapi_library_name_;
#endif

  // These are set on the UI thread, and then consumed during initialization on
  // the IO thread.
  content::mojom::NetworkContextRequest network_context_request_;
  content::mojom::NetworkContextParamsPtr network_context_params_;

  // Returns a getter for the URLRequestContext.  Only called on the UI thread.
  net::URLRequestContextGetter* system_url_request_context_getter();

  // For a description of what these features are, and how they are
  // configured, see the comments in pref_names.cc for
  // |kQuickCheckEnabled| and |kPacHttpsUrlStrippingEnabled
  // respectively.
  bool WpadQuickCheckEnabled() const;
  bool PacHttpsUrlStrippingEnabled() const;

  // Configures |builder|'s ProxyService to use the specified
  // |proxy_config_service| and sets a number of proxy-related options based on
  // prefs, policies, and the command line.
  void SetUpProxyConfigService(
      content::URLRequestContextBuilderMojo* builder,
      std::unique_ptr<net::ProxyConfigService> proxy_config_service) const;

private:
  void UpdateServerWhitelist();
  void UpdateDelegateWhitelist();
  void UpdateNegotiateDisableCnameLookup();
  void UpdateNegotiateEnablePort();

  void ConstructSystemRequestContext();


  // The NetLog is owned by the browser process, to allow logging from other
  // threads during shutdown, but is used most frequently on the IOThread.
  net_log::ChromeNetLog* net_log_;

  // These member variables are basically global, but their lifetimes are tied
  // to the IOThread.  IOThread owns them all, despite not using scoped_ptr.
  // This is because the destructor of IOThread runs on the wrong thread.  All
  // member variables should be deleted in CleanUp().

  // These member variables are initialized in Init() and do not change for the
  // lifetime of the IO thread.
  Globals* globals_;

  std::unique_ptr<certificate_transparency::TreeStateTracker> ct_tree_tracker_;

  // These member variables are initialized by a task posted to the IO thread,
  // which gets posted by calling certain member functions of IOThread.
  std::unique_ptr<net::ProxyConfigService> system_proxy_config_service_;

  std::unique_ptr<PrefProxyConfigTracker> pref_proxy_config_tracker_;

  scoped_refptr<net::URLRequestContextGetter>
      system_url_request_context_getter_;

  BooleanPrefMember quick_check_enabled_;

  BooleanPrefMember pac_https_url_stripping_enabled_;

  // Store HTTP Auth-related policies in this thread.
  // TODO(aberent) Make the list of auth schemes a PrefMember, so that the
  // policy can change after startup (https://crbug/549273).
  std::string auth_schemes_;
  BooleanPrefMember negotiate_disable_cname_lookup_;
  BooleanPrefMember negotiate_enable_port_;
  StringPrefMember auth_server_whitelist_;
  StringPrefMember auth_delegate_whitelist_;

  // This is an instance of the default SSLConfigServiceManager for the current
  // platform and it gets SSL preferences from local_state object.
  std::unique_ptr<ssl_config::SSLConfigServiceManager>
      ssl_config_service_manager_;

  DISALLOW_COPY_AND_ASSIGN(IOThread);
};

#endif  // CHROME_BROWSER_IO_THREAD_H_
