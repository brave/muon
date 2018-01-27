// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/io_thread.h"

#include "atom/browser/net/atom_network_delegate.h"
#include "base/command_line.h"
#include "base/debug/leak_tracker.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/data_use_measurement/chrome_data_use_ascriber.h"
#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"
#include "chrome/browser/net/dns_probe_service.h"
#include "chrome/browser/net/proxy_service_factory.h"
#include "chrome/browser/net/sth_distributor_provider.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/certificate_transparency/tree_state_tracker.h"
#include "components/data_usage/core/data_use_aggregator.h"
#include "components/net_log/chrome_net_log.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/pref_proxy_config_tracker.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "content/public/network/ignore_errors_cert_verifier.h"
#include "content/public/network/url_request_context_builder_mojo.h"
#include "extensions/features/features.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_known_logs.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_verifier.h"
#include "net/cert/sth_distributor.h"
#include "net/cert/sth_observer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "vendor/brightray/common/content_client.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/event_router_forwarder.h"
#endif

#if defined(USE_NSS_CERTS)
#include "net/cert_net/nss_ocsp.h"
#endif

using content::BrowserThread;

namespace {

#if defined(OS_MACOSX)
void ObserveKeychainEvents() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  net::CertDatabase::GetInstance()->SetMessageLoopForKeychainEvents();
}
#endif

}  // namespace

IOThread::IOThread(
    PrefService* local_state,
    policy::PolicyService* policy_service,
    net_log::ChromeNetLog* net_log,
    extensions::EventRouterForwarder* extension_event_router_forwarder,
    SystemNetworkContextManager* system_network_context_manager)
    : net_log_(net_log),
#if BUILDFLAG(ENABLE_EXTENSIONS)
      extension_event_router_forwarder_(extension_event_router_forwarder),
#endif
      globals_(nullptr),
      is_quic_allowed_on_init_(false),
      network_service_request_(mojo::MakeRequest(&ui_thread_network_service_)),
      weak_factory_(this) {
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_proxy =
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
  auth_schemes_ = local_state->GetString(prefs::kAuthSchemes);
  negotiate_disable_cname_lookup_.Init(
      prefs::kDisableAuthNegotiateCnameLookup, local_state,
      base::Bind(&IOThread::UpdateNegotiateDisableCnameLookup,
                 base::Unretained(this)));
  negotiate_disable_cname_lookup_.MoveToThread(io_thread_proxy);
  negotiate_enable_port_.Init(
      prefs::kEnableAuthNegotiatePort, local_state,
      base::Bind(&IOThread::UpdateNegotiateEnablePort, base::Unretained(this)));
  negotiate_enable_port_.MoveToThread(io_thread_proxy);
  auth_server_whitelist_.Init(
      prefs::kAuthServerWhitelist, local_state,
      base::Bind(&IOThread::UpdateServerWhitelist, base::Unretained(this)));
  auth_server_whitelist_.MoveToThread(io_thread_proxy);
  auth_delegate_whitelist_.Init(
      prefs::kAuthNegotiateDelegateWhitelist, local_state,
      base::Bind(&IOThread::UpdateDelegateWhitelist, base::Unretained(this)));
  auth_delegate_whitelist_.MoveToThread(io_thread_proxy);
#if defined(OS_POSIX)
  gssapi_library_name_ = local_state->GetString(prefs::kGSSAPILibraryName);
#endif
  pref_proxy_config_tracker_.reset(
      ProxyServiceFactory::CreatePrefProxyConfigTrackerOfLocalState(
          local_state));
  system_proxy_config_service_ = ProxyServiceFactory::CreateProxyConfigService(
      pref_proxy_config_tracker_.get());
  ssl_config_service_manager_.reset(
      ssl_config::SSLConfigServiceManager::CreateDefaultManager(
          local_state,
          BrowserThread::GetTaskRunnerForThread(BrowserThread::IO)));

  local_state->SetDefaultPrefValue(
      prefs::kBuiltInDnsClientEnabled,
      base::Value(base::FeatureList::IsEnabled(features::kAsyncDns)));

  dns_client_enabled_.Init(prefs::kBuiltInDnsClientEnabled,
                           local_state,
                           base::Bind(&IOThread::UpdateDnsClientEnabled,
                                      base::Unretained(this)));
  dns_client_enabled_.MoveToThread(io_thread_proxy);

  quick_check_enabled_.Init(prefs::kQuickCheckEnabled,
                            local_state);
  quick_check_enabled_.MoveToThread(io_thread_proxy);

  pac_https_url_stripping_enabled_.Init(prefs::kPacHttpsUrlStrippingEnabled,
                                        local_state);
  pac_https_url_stripping_enabled_.MoveToThread(io_thread_proxy);

  chrome_browser_net::SetGlobalSTHDistributor(
      std::unique_ptr<net::ct::STHDistributor>(new net::ct::STHDistributor()));

  BrowserThread::SetIOThreadDelegate(this);

  system_network_context_manager->SetUp(&network_context_request_,
                                        &network_context_params_,
                                        &is_quic_allowed_on_init_);
}

IOThread::~IOThread() {
  BrowserThread::SetIOThreadDelegate(nullptr);

  pref_proxy_config_tracker_->DetachFromPrefService();
  DCHECK(!globals_);

  // Destroy the old distributor to check that the observers list it holds is
  // empty.
  chrome_browser_net::SetGlobalSTHDistributor(nullptr);
}

IOThread::Globals* IOThread::globals() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return globals_;
}

IOThread::Globals::
SystemRequestContextLeakChecker::SystemRequestContextLeakChecker(
    Globals* globals)
    : globals_(globals) {
  DCHECK(globals_);
}

IOThread::Globals::
SystemRequestContextLeakChecker::~SystemRequestContextLeakChecker() {
  globals_->system_request_context->AssertNoURLRequests();
}

IOThread::Globals::Globals()
    : system_request_context(nullptr),
      system_request_context_leak_checker(this) {}

IOThread::Globals::~Globals() {}

class SystemURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  explicit SystemURLRequestContextGetter(IOThread* io_thread);

  // Implementation for net::UrlRequestContextGetter.
  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

 protected:
  ~SystemURLRequestContextGetter() override;

 private:
  IOThread* const io_thread_;  // Weak pointer, owned by BrowserProcess.
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;

  base::debug::LeakTracker<SystemURLRequestContextGetter> leak_tracker_;
};

SystemURLRequestContextGetter::SystemURLRequestContextGetter(
    IOThread* io_thread)
    : io_thread_(io_thread),
      network_task_runner_(
          BrowserThread::GetTaskRunnerForThread(BrowserThread::IO)) {}

SystemURLRequestContextGetter::~SystemURLRequestContextGetter() {}

net::URLRequestContext* SystemURLRequestContextGetter::GetURLRequestContext() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(io_thread_->globals()->system_request_context);

  return io_thread_->globals()->system_request_context;
}

scoped_refptr<base::SingleThreadTaskRunner>
SystemURLRequestContextGetter::GetNetworkTaskRunner() const {
  return network_task_runner_;
}

void IOThread::Init() {
  TRACE_EVENT0("startup", "IOThread::InitAsync");
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

#if defined(USE_NSS_CERTS)
  net::SetMessageLoopForNSSHttpIO();
#endif

  DCHECK(!globals_);
  globals_ = new Globals;

  // Setup the HistogramWatcher to run on the IO thread.
  net::NetworkChangeNotifier::InitHistogramWatcher();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  globals_->extension_event_router_forwarder =
      extension_event_router_forwarder_;
#endif

  std::vector<scoped_refptr<const net::CTLogVerifier>> ct_logs(
      net::ct::CreateLogVerifiersForKnownLogs());

  globals_->ct_logs.assign(ct_logs.begin(), ct_logs.end());

  ct_tree_tracker_.reset(new certificate_transparency::TreeStateTracker(
      globals_->ct_logs, net_log_));
  // Register the ct_tree_tracker_ as observer for new STHs.
  RegisterSTHObserver(ct_tree_tracker_.get());

#if defined(OS_MACOSX)
  // Start observing Keychain events. This needs to be done on the UI thread,
  // as Keychain services requires a CFRunLoop.
  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&ObserveKeychainEvents));
#endif

  ConstructSystemRequestContext();

  UpdateDnsClientEnabled();
}

void IOThread::CleanUp() {
#if defined(USE_NSS_CERTS)
  net::ShutdownNSSHttpIO();
#endif

  system_url_request_context_getter_ = NULL;

  // Unlink the ct_tree_tracker_ from the global cert_transparency_verifier
  // and unregister it from new STH notifications so it will take no actions
  // on anything observed during CleanUp process.
  globals()->system_request_context->cert_transparency_verifier()->SetObserver(
      nullptr);
  UnregisterSTHObserver(ct_tree_tracker_.get());
  ct_tree_tracker_.reset();

  globals_->system_request_context->proxy_service()->OnShutdown();

#if defined(USE_NSS_CERTS)
  net::SetURLRequestContextForNSSHttpIO(nullptr);
#endif

  // Shutdown the HistogramWatcher on the IO thread.
  net::NetworkChangeNotifier::ShutdownHistogramWatcher();

  system_proxy_config_service_.reset();
  delete globals_;
  globals_ = NULL;

  base::debug::LeakTracker<SystemURLRequestContextGetter>::CheckForLeaks();

  if (net_log_)
    net_log_->ShutDownBeforeTaskScheduler();
}

void IOThread::ChangedToOnTheRecord() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&IOThread::ChangedToOnTheRecordOnIOThread,
                     base::Unretained(this)));
}

net::URLRequestContextGetter* IOThread::system_url_request_context_getter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!system_url_request_context_getter_.get()) {
    system_url_request_context_getter_ =
        new SystemURLRequestContextGetter(this);
  }
  return system_url_request_context_getter_.get();
}

void IOThread::ClearHostCache(
    const base::Callback<bool(const std::string&)>& host_filter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  globals_->system_request_context->host_resolver()
      ->GetHostCache()
      ->ClearForHosts(host_filter);
}

void IOThread::DisableQuic() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  globals_->network_service->DisableQuic();
}

net::SSLConfigService* IOThread::GetSSLConfigService() {
  return ssl_config_service_manager_->Get();
}

void IOThread::ChangedToOnTheRecordOnIOThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Clear the host cache to avoid showing entries from the OTR session
  // in about:net-internals.
  ClearHostCache(base::Callback<bool(const std::string&)>());
}

// static

// static
void IOThread::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kAuthSchemes,
                               "basic,digest,ntlm,negotiate");
  registry->RegisterBooleanPref(prefs::kDisableAuthNegotiateCnameLookup, false);
  registry->RegisterBooleanPref(prefs::kEnableAuthNegotiatePort, false);
  registry->RegisterStringPref(prefs::kAuthServerWhitelist, std::string());
  registry->RegisterStringPref(prefs::kAuthNegotiateDelegateWhitelist,
                               std::string());
  registry->RegisterStringPref(prefs::kGSSAPILibraryName, std::string());
  registry->RegisterStringPref(prefs::kAuthAndroidNegotiateAccountType,
                               std::string());
  registry->RegisterBooleanPref(prefs::kEnableReferrers, true);
  registry->RegisterBooleanPref(prefs::kBuiltInDnsClientEnabled, true);
  registry->RegisterBooleanPref(prefs::kQuickCheckEnabled, true);
  registry->RegisterBooleanPref(prefs::kPacHttpsUrlStrippingEnabled, true);
}

void IOThread::UpdateServerWhitelist() {
  globals_->http_auth_preferences->SetServerWhitelist(
      auth_server_whitelist_.GetValue());
}

void IOThread::UpdateDelegateWhitelist() {
  globals_->http_auth_preferences->SetDelegateWhitelist(
      auth_delegate_whitelist_.GetValue());
}

void IOThread::UpdateNegotiateDisableCnameLookup() {
  globals_->http_auth_preferences->set_negotiate_disable_cname_lookup(
      negotiate_disable_cname_lookup_.GetValue());
}

void IOThread::UpdateNegotiateEnablePort() {
  globals_->http_auth_preferences->set_negotiate_enable_port(
      negotiate_enable_port_.GetValue());
}

void IOThread::UpdateDnsClientEnabled() {
  globals()->system_request_context->host_resolver()->SetDnsClientEnabled(
      *dns_client_enabled_);
}

void IOThread::RegisterSTHObserver(net::ct::STHObserver* observer) {
  chrome_browser_net::GetGlobalSTHDistributor()->RegisterObserver(observer);
}

void IOThread::UnregisterSTHObserver(net::ct::STHObserver* observer) {
  chrome_browser_net::GetGlobalSTHDistributor()->UnregisterObserver(observer);
}

bool IOThread::WpadQuickCheckEnabled() const {
  return quick_check_enabled_.GetValue();
}

bool IOThread::PacHttpsUrlStrippingEnabled() const {
  return pac_https_url_stripping_enabled_.GetValue();
}

std::unique_ptr<net::HostResolver> CreateGlobalHostResolver(
    net::NetLog* net_log) {
  TRACE_EVENT0("startup", "IOThread::CreateGlobalHostResolver");
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  net::HostResolver::Options options;
  std::unique_ptr<net::HostResolver> global_host_resolver;
  global_host_resolver =
      net::HostResolver::CreateSystemResolver(options, net_log);

  // If hostname remappings were specified on the command-line, layer these
  // rules on top of the real host resolver. This allows forwarding all requests
  // through a designated test server.
  if (!command_line.HasSwitch(switches::kHostResolverRules))
    return global_host_resolver;

  std::unique_ptr<net::MappedHostResolver> remapped_resolver(
      new net::MappedHostResolver(std::move(global_host_resolver)));
  remapped_resolver->SetRulesFromString(
      command_line.GetSwitchValueASCII(switches::kHostResolverRules));
  return std::move(remapped_resolver);
}

std::unique_ptr<net::HttpAuthHandlerFactory>
IOThread::CreateDefaultAuthHandlerFactory(net::HostResolver* host_resolver) {
  std::vector<std::string> supported_schemes = base::SplitString(
      auth_schemes_, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  globals_->http_auth_preferences.reset(new net::HttpAuthPreferences(
      supported_schemes
#if defined(OS_POSIX) && !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
      ,
      gssapi_library_name_
#endif
      ));
  UpdateServerWhitelist();
  UpdateDelegateWhitelist();
  UpdateNegotiateDisableCnameLookup();
  UpdateNegotiateEnablePort();

  return net::HttpAuthHandlerRegistryFactory::Create(
      globals_->http_auth_preferences.get(), host_resolver);
}

void IOThread::SetUpProxyConfigService(
    content::URLRequestContextBuilderMojo* builder,
    std::unique_ptr<net::ProxyConfigService> proxy_config_service) const {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  // TODO(eroman): Figure out why this doesn't work in single-process mode.
  // Should be possible now that a private isolate is used.
  // http://crbug.com/474654
  if (!command_line.HasSwitch(switches::kWinHttpProxyResolver)) {
    if (command_line.HasSwitch(switches::kSingleProcess)) {
      LOG(ERROR) << "Cannot use V8 Proxy resolver in single process mode.";
    } else {
      builder->SetMojoProxyResolverFactory(
          ChromeMojoProxyResolverFactory::CreateWithStrongBinding());
    }
  }

  builder->set_pac_quick_check_enabled(WpadQuickCheckEnabled());
  builder->set_pac_sanitize_url_policy(
      PacHttpsUrlStrippingEnabled()
          ? net::ProxyService::SanitizeUrlPolicy::SAFE
          : net::ProxyService::SanitizeUrlPolicy::UNSAFE);
  builder->set_proxy_config_service(std::move(proxy_config_service));
}

void IOThread::ConstructSystemRequestContext() {
  std::unique_ptr<content::URLRequestContextBuilderMojo> builder =
      base::MakeUnique<content::URLRequestContextBuilderMojo>();

  builder->set_user_agent(GetUserAgent());
  std::unique_ptr<atom::AtomNetworkDelegate>
      atom_network_delegate(new atom::AtomNetworkDelegate());

  builder->set_network_delegate(std::move(atom_network_delegate));
  std::unique_ptr<net::HostResolver> host_resolver(
      CreateGlobalHostResolver(net_log_));

  builder->set_ssl_config_service(GetSSLConfigService());
  builder->SetHttpAuthHandlerFactory(
      CreateDefaultAuthHandlerFactory(host_resolver.get()));

  builder->set_host_resolver(std::move(host_resolver));

  std::unique_ptr<net::CertVerifier> cert_verifier;
  cert_verifier = net::CertVerifier::CreateDefault();

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  builder->SetCertVerifier(
      content::IgnoreErrorsCertVerifier::MaybeWrapCertVerifier(
          command_line, switches::kUserDataDir, std::move(cert_verifier)));

  std::unique_ptr<net::MultiLogCTVerifier> ct_verifier =
      base::MakeUnique<net::MultiLogCTVerifier>();
  // Add built-in logs
  ct_verifier->AddLogs(globals_->ct_logs);

  // Register the ct_tree_tracker_ as observer for verified SCTs.
  ct_verifier->SetObserver(ct_tree_tracker_.get());

  builder->set_ct_verifier(std::move(ct_verifier));

  SetUpProxyConfigService(builder.get(),
                          std::move(system_proxy_config_service_));

  globals_->network_service = content::NetworkService::Create(
      std::move(network_service_request_), net_log_);
  globals_->network_service->DisableQuic();

  globals_->system_network_context =
      globals_->network_service->CreateNetworkContextWithBuilder(
          std::move(network_context_request_),
          std::move(network_context_params_), std::move(builder),
          &globals_->system_request_context);

#if defined(USE_NSS_CERTS)
  net::SetURLRequestContextForNSSHttpIO(globals_->system_request_context);
#endif
}
