// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/io_thread.h"

#include "atom/browser/net/atom_network_delegate.h"
#include "base/command_line.h"
#include "base/debug/leak_tracker.h"
#include "base/trace_event/trace_event.h"
#include "chrome/browser/data_use_measurement/chrome_data_use_ascriber.h"
#include "chrome/browser/net/dns_probe_service.h"
#include "chrome/browser/net/failing_url_request_interceptor.h"
#include "chrome/browser/net/proxy_service_factory.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/data_usage/core/data_use_aggregator.h"
#include "components/net_log/chrome_net_log.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "extensions/buildflags/buildflags.h"
#include "net/cert/caching_cert_verifier.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/cert_verify_proc.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/multi_threaded_cert_verifier.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/nqe/network_quality_estimator.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/ignore_errors_cert_verifier.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/network_switches.h"
#include "services/network/url_request_context_builder_mojo.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/event_router_forwarder.h"
#endif

#if defined(USE_NSS_CERTS)
#include "net/cert_net/nss_ocsp.h"
#endif

using content::BrowserThread;

namespace {

net::CertVerifier* g_cert_verifier_for_io_thread_testing = nullptr;

// A CertVerifier that forwards all requests to
// |g_cert_verifier_for_io_thread_testing|. This is used to allow IOThread to
// have its own std::unique_ptr<net::CertVerifier> while forwarding calls to the
// static verifier.
class WrappedCertVerifierForIOThreadTesting : public net::CertVerifier {
 public:
  ~WrappedCertVerifierForIOThreadTesting() override = default;

  // CertVerifier implementation
  int Verify(const RequestParams& params,
             net::CRLSet* crl_set,
             net::CertVerifyResult* verify_result,
             net::CompletionOnceCallback callback,
             std::unique_ptr<Request>* out_req,
             const net::NetLogWithSource& net_log) override {
    verify_result->Reset();
    if (!g_cert_verifier_for_io_thread_testing)
      return net::ERR_FAILED;
    return g_cert_verifier_for_io_thread_testing->Verify(
        params, crl_set, verify_result, std::move(callback), out_req, net_log);
  }
};

#if defined(OS_MACOSX)
void ObserveKeychainEvents() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  net::CertDatabase::GetInstance()->SetMessageLoopForKeychainEvents();
}
#endif

std::unique_ptr<net::HostResolver> CreateGlobalHostResolver(
    net::NetLog* net_log) {
  TRACE_EVENT0("startup", "IOThread::CreateGlobalHostResolver");

  using resolver = net::HostResolver;
  std::unique_ptr<net::HostResolver> global_host_resolver =
      resolver::CreateSystemResolver(net::HostResolver::Options(), net_log);

  // If hostname remappings were specified on the command-line, layer these
  // rules on top of the real host resolver. This allows forwarding all requests
  // through a designated test server.
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (!command_line.HasSwitch(network::switches::kHostResolverRules))
    return global_host_resolver;

  auto remapped_resolver = std::make_unique<net::MappedHostResolver>(
      std::move(global_host_resolver));
  remapped_resolver->SetRulesFromString(
      command_line.GetSwitchValueASCII(network::switches::kHostResolverRules));
  return std::move(remapped_resolver);
}

}  // namespace

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

IOThread::Globals::Globals() : system_request_context(nullptr) {}

IOThread::Globals::~Globals() {}

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
      weak_factory_(this) {
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_proxy =
      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);

  BrowserThread::SetIOThreadDelegate(this);

  system_network_context_manager->SetUp(
      &network_context_request_, &network_context_params_,
      &stub_resolver_enabled_, &dns_over_https_servers_,
      &http_auth_static_params_, &http_auth_dynamic_params_,
      &is_quic_allowed_on_init_);
}

IOThread::~IOThread() {
  BrowserThread::SetIOThreadDelegate(nullptr);

  DCHECK(!globals_);
}

IOThread::Globals* IOThread::globals() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return globals_;
}

net::URLRequestContextGetter* IOThread::system_url_request_context_getter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!system_url_request_context_getter_.get()) {
    system_url_request_context_getter_ =
        base::MakeRefCounted<SystemURLRequestContextGetter>(this);
  }
  return system_url_request_context_getter_.get();
}

void IOThread::Init() {
  TRACE_EVENT0("startup", "IOThread::InitAsync");
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DCHECK(!globals_);
  globals_ = new Globals;

  // Setup the HistogramWatcher to run on the IO thread.
  net::NetworkChangeNotifier::InitHistogramWatcher();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  globals_->extension_event_router_forwarder =
      extension_event_router_forwarder_;
#endif

#if defined(OS_MACOSX)
  // Start observing Keychain events. This needs to be done on the UI thread,
  // as Keychain services requires a CFRunLoop.
  BrowserThread::PostTask(BrowserThread::UI,
                          FROM_HERE,
                          base::Bind(&ObserveKeychainEvents));
#endif

  ConstructSystemRequestContext();
}

void IOThread::CleanUp() {
  system_url_request_context_getter_ = nullptr;

  globals_->system_request_context->proxy_resolution_service()->OnShutdown();

#if defined(USE_NSS_CERTS)
  net::SetURLRequestContextForNSSHttpIO(nullptr);
#endif

  // Shutdown the HistogramWatcher on the IO thread.
  net::NetworkChangeNotifier::ShutdownHistogramWatcher();

  delete globals_;
  globals_ = nullptr;

  base::debug::LeakTracker<SystemURLRequestContextGetter>::CheckForLeaks();

  if (net_log_)
    net_log_->ShutDownBeforeTaskScheduler();
}

// static
void IOThread::RegisterPrefs(PrefRegistrySimple* registry) {}

void IOThread::DisableQuic() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  globals_->quic_disabled = true;
}

void IOThread::SetUpProxyService(
    network::URLRequestContextBuilderMojo* builder) const {}

void IOThread::ConstructSystemRequestContext() {
  if (base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    globals_->deprecated_network_quality_estimator =
        std::make_unique<net::NetworkQualityEstimator>(
            std::make_unique<net::NetworkQualityEstimatorParams>(
                std::map<std::string, std::string>()),
            net_log_);
    net::URLRequestContextBuilder builder;
    std::vector<std::unique_ptr<net::URLRequestInterceptor>>
        url_request_interceptors;
    url_request_interceptors.emplace_back(
        std::make_unique<FailingURLRequestInterceptor>());
    builder.SetInterceptors(std::move(url_request_interceptors));
    builder.set_network_quality_estimator(
        globals_->deprecated_network_quality_estimator.get());
    builder.SetCertVerifier(
        std::make_unique<WrappedCertVerifierForIOThreadTesting>());
    builder.set_proxy_resolution_service(
        net::ProxyResolutionService::CreateDirect());
    globals_->system_request_context_owner =
        network::URLRequestContextOwner(nullptr, builder.Build());
    globals_->system_request_context =
        globals_->system_request_context_owner.url_request_context.get();
    network_context_params_.reset();
  } else {
    std::unique_ptr<network::URLRequestContextBuilderMojo> builder =
        std::make_unique<network::URLRequestContextBuilderMojo>();

    std::unique_ptr<atom::AtomNetworkDelegate> atom_network_delegate =
        std::make_unique<atom::AtomNetworkDelegate>();
    builder->set_network_delegate(std::move(atom_network_delegate));

    std::unique_ptr<net::CertVerifier> cert_verifier;
    if (g_cert_verifier_for_io_thread_testing) {
      cert_verifier = std::make_unique<WrappedCertVerifierForIOThreadTesting>();
    } else {
      cert_verifier = std::make_unique<net::CachingCertVerifier>(
          std::make_unique<net::MultiThreadedCertVerifier>(
              net::CertVerifyProc::CreateDefault()));
    }
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();
    builder->SetCertVerifier(
        network::IgnoreErrorsCertVerifier::MaybeWrapCertVerifier(
            command_line, switches::kUserDataDir, std::move(cert_verifier)));

    SetUpProxyService(builder.get());

    if (!is_quic_allowed_on_init_)
      globals_->quic_disabled = true;

    network::NetworkService* network_service = content::GetNetworkServiceImpl();
    network_service->SetHostResolver(CreateGlobalHostResolver(net_log_));

    // These must be done after the SetHostResolver call.
    network_service->SetUpHttpAuth(std::move(http_auth_static_params_));
    network_service->ConfigureHttpAuthPrefs(
        std::move(http_auth_dynamic_params_));

    globals_->system_network_context =
        network_service->CreateNetworkContextWithBuilder(
            std::move(network_context_request_),
            std::move(network_context_params_), std::move(builder),
            &globals_->system_request_context);

    // This must be done after the system NetworkContext is created.
    network_service->ConfigureStubHostResolver(
        stub_resolver_enabled_, std::move(dns_over_https_servers_));
  }


#if defined(USE_NSS_CERTS)
  net::SetURLRequestContextForNSSHttpIO(globals_->system_request_context);
#endif
}
