// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "browser/url_request_context_getter.h"

#include <algorithm>

#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_worker_pool.h"
#include "browser/net_log.h"
#include "browser/network_delegate.h"
#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"
#include "chrome/common/chrome_switches.h"
#include "common/switches.h"
#include "components/cookie_config/cookie_store_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/browser/devtools_network_transaction_factory.h"
#include "content/public/common/content_switches.h"
#include "net/base/host_mapping_rules.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_known_logs.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cookies/cookie_monster.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/http/http_auth_filter.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/http/http_server_properties_impl.h"
#include "net/log/net_log.h"
#include "net/proxy_resolution/dhcp_pac_file_fetcher_factory.h"
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/pac_file_fetcher_impl.h"
#include "net/proxy_resolution/proxy_service.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "services/network/proxy_service_mojo.h"
#include "storage/browser/quota/special_storage_policy.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_constants.h"

#if defined(USE_NSS_CERTS)
#include "net/cert_net/nss_ocsp.h"
#endif

using content::BrowserThread;

namespace brightray {

std::string URLRequestContextGetter::Delegate::GetUserAgent() {
  return base::EmptyString();
}

std::unique_ptr<net::URLRequestJobFactory>
URLRequestContextGetter::Delegate::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* protocol_handlers) {
  std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(new net::URLRequestJobFactoryImpl);

  for (auto& it : *protocol_handlers) {
    job_factory->SetProtocolHandler(
        it.first, base::WrapUnique(it.second.release()));
  }
  protocol_handlers->clear();

  job_factory->SetProtocolHandler(
      url::kDataScheme, base::WrapUnique(new net::DataProtocolHandler));
  job_factory->SetProtocolHandler(
      url::kFileScheme,
      base::WrapUnique(
          new net::FileProtocolHandler(base::CreateTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
               base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}))));

  return std::move(job_factory);
}

net::HttpCache::BackendFactory*
URLRequestContextGetter::Delegate::CreateHttpCacheBackendFactory(const base::FilePath& base_path) {
  base::FilePath cache_path = base_path.Append(FILE_PATH_LITERAL("Cache"));
  return new net::HttpCache::DefaultBackend(
      net::DISK_CACHE, net::CACHE_BACKEND_DEFAULT, cache_path, 0);
}

std::unique_ptr<net::CertVerifier>
URLRequestContextGetter::Delegate::CreateCertVerifier() {
  return net::CertVerifier::CreateDefault();
}

net::SSLConfigService* URLRequestContextGetter::Delegate::CreateSSLConfigService() {
  return new net::SSLConfigServiceDefaults;
}

std::vector<std::string> URLRequestContextGetter::Delegate::GetCookieableSchemes() {
  return { "http", "https", "ws", "wss" };
}

URLRequestContextGetter::URLRequestContextGetter(
    Delegate* delegate,
    NetLog* net_log,
    const base::FilePath& base_path,
    bool in_memory,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> file_task_runner,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector protocol_interceptors)
    : delegate_(delegate),
      net_log_(net_log),
      base_path_(base_path),
      in_memory_(in_memory),
      io_task_runner_(io_task_runner),
      file_task_runner_(file_task_runner),
      protocol_interceptors_(std::move(protocol_interceptors)),
      job_factory_(nullptr),
      shutting_down_(false) {
  // Must first be created on the UI thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (protocol_handlers)
    std::swap(protocol_handlers_, *protocol_handlers);

  if (delegate_)
    user_agent_ = delegate_->GetUserAgent();

  // We must create the proxy config service on the UI loop on Linux because it
  // must synchronously run on the glib message loop. This will be passed to
  // the URLRequestContextStorage on the IO thread in GetURLRequestContext().
  proxy_config_service_ = net::ProxyResolutionService::CreateSystemProxyConfigService(
      file_task_runner_);
}

URLRequestContextGetter::~URLRequestContextGetter() {}

void URLRequestContextGetter::NotifyContextShuttingDown() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  shutting_down_ = true;

  net::URLRequestContextGetter::NotifyContextShuttingDown();
}

net::HostResolver* URLRequestContextGetter::host_resolver() {
  if (shutting_down_) {
    return NULL;
  }

  return url_request_context_->host_resolver();
}

net::URLRequestContext* URLRequestContextGetter::GetURLRequestContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (shutting_down_) {
    return NULL;
  }

  if (!url_request_context_.get()) {
    auto& command_line = *base::CommandLine::ForCurrentProcess();
    url_request_context_.reset(new net::URLRequestContext);

    // --log-net-log
    if (net_log_) {
      net_log_->StartLogging();
      url_request_context_->set_net_log(net_log_);
    }

    network_delegate_.reset(delegate_->CreateNetworkDelegate());
    url_request_context_->set_network_delegate(network_delegate_.get());

    storage_.reset(new net::URLRequestContextStorage(url_request_context_.get()));

    std::unique_ptr<net::CookieStore> cookie_store = nullptr;
    if (in_memory_) {
      auto cookie_config = content::CookieStoreConfig();
      cookie_config.cookieable_schemes = delegate_->GetCookieableSchemes();
      cookie_config.crypto_delegate = cookie_config::GetCookieCryptoDelegate();
      cookie_store = content::CreateCookieStore(cookie_config);
    } else {
      auto cookie_config = content::CookieStoreConfig(
          base_path_.Append(FILE_PATH_LITERAL("Cookies")),
          false /* restore_old_session_cookies */,
          false /* persist_session_cookies */, nullptr);
      cookie_config.cookieable_schemes = delegate_->GetCookieableSchemes();
      cookie_config.crypto_delegate = cookie_config::GetCookieCryptoDelegate();
      cookie_store = content::CreateCookieStore(cookie_config);
    }
    storage_->set_cookie_store(std::move(cookie_store));
    storage_->set_channel_id_service(base::WrapUnique(
        new net::ChannelIDService(new net::DefaultChannelIDStore(nullptr))));

    std::string accept_lang = l10n_util::GetApplicationLocale("");
    storage_->set_http_user_agent_settings(base::WrapUnique(
        new net::StaticHttpUserAgentSettings(
            net::HttpUtil::GenerateAcceptLanguageHeader(accept_lang),
            user_agent_)));

    std::unique_ptr<net::HostResolver> host_resolver(net::HostResolver::CreateDefaultResolver(nullptr));

    // --host-resolver-rules
    if (command_line.HasSwitch(::switches::kHostResolverRules)) {
      std::unique_ptr<net::MappedHostResolver> remapped_resolver(
          new net::MappedHostResolver(std::move(host_resolver)));
      remapped_resolver->SetRulesFromString(
          command_line.GetSwitchValueASCII(::switches::kHostResolverRules));
      host_resolver = std::move(remapped_resolver);
    }

    // --proxy-server
    if (command_line.HasSwitch(switches::kNoProxyServer)) {
      storage_->set_proxy_resolution_service(net::ProxyResolutionService::CreateDirect());
    } else if (command_line.HasSwitch(switches::kProxyServer)) {
      net::ProxyConfig proxy_config;
      proxy_config.proxy_rules().ParseFromString(
          command_line.GetSwitchValueASCII(switches::kProxyServer));
      proxy_config.proxy_rules().bypass_rules.ParseFromString(
          command_line.GetSwitchValueASCII(switches::kProxyBypassList));
      storage_->set_proxy_resolution_service(net::ProxyResolutionService::CreateFixed(proxy_config));
    } else if (command_line.HasSwitch(switches::kProxyPacUrl)) {
      auto proxy_config = net::ProxyConfig::CreateFromCustomPacURL(
          GURL(command_line.GetSwitchValueASCII(switches::kProxyPacUrl)));
      proxy_config.set_pac_mandatory(true);
      storage_->set_proxy_resolution_service(net::ProxyResolutionService::CreateFixed(
          proxy_config));
    } else {
      bool use_v8 = !command_line.HasSwitch(::switches::kWinHttpProxyResolver);

      std::unique_ptr<net::ProxyResolutionService> proxy_service;
      if (use_v8) {
        std::unique_ptr<net::DhcpProxyScriptFetcher> dhcp_proxy_script_fetcher;
        net::DhcpProxyScriptFetcherFactory dhcp_factory;
        dhcp_proxy_script_fetcher = dhcp_factory.Create(url_request_context_.get());

        proxy_service = network::CreateProxyServiceUsingMojoFactory(
            ChromeMojoProxyResolverFactory::CreateWithStrongBinding(),
            std::move(proxy_config_service_),
            std::make_unique<net::ProxyScriptFetcherImpl>(
                url_request_context_.get()),
            std::move(dhcp_proxy_script_fetcher), host_resolver.get(), net_log_,
            url_request_context_->network_delegate());
      } else {
        proxy_service = net::ProxyResolutionService::CreateUsingSystemProxyResolver(
            std::move(proxy_config_service_),
            net_log_);
      }

      proxy_service->set_quick_check_enabled(true);
      proxy_service->set_sanitize_url_policy(
          net::ProxyResolutionService::SanitizeUrlPolicy::SAFE);

      storage_->set_proxy_resolution_service(std::move(proxy_service));
    }

    std::vector<std::string> schemes;
    schemes.push_back(std::string("basic"));
    schemes.push_back(std::string("digest"));
    schemes.push_back(std::string("ntlm"));
    schemes.push_back(std::string("negotiate"));
#if defined(OS_POSIX)
    http_auth_preferences_.reset(new net::HttpAuthPreferences(schemes,
                                                              std::string()));
#else
    http_auth_preferences_.reset(new net::HttpAuthPreferences(schemes));
#endif

    // --auth-server-whitelist
    if (command_line.HasSwitch(switches::kAuthServerWhitelist)) {
      http_auth_preferences_->SetServerWhitelist(
          command_line.GetSwitchValueASCII(switches::kAuthServerWhitelist));
    }

    // --auth-negotiate-delegate-whitelist
    if (command_line.HasSwitch(switches::kAuthNegotiateDelegateWhitelist)) {
      http_auth_preferences_->SetDelegateWhitelist(
          command_line.GetSwitchValueASCII(
              switches::kAuthNegotiateDelegateWhitelist));
    }

    auto auth_handler_factory =
        net::HttpAuthHandlerRegistryFactory::Create(
            http_auth_preferences_.get(), host_resolver.get());

    storage_->set_cert_verifier(delegate_->CreateCertVerifier());
    storage_->set_transport_security_state(
        base::WrapUnique(new net::TransportSecurityState));
    storage_->set_ssl_config_service(delegate_->CreateSSLConfigService());
    storage_->set_http_auth_handler_factory(std::move(auth_handler_factory));
    std::unique_ptr<net::HttpServerProperties> server_properties(
        new net::HttpServerPropertiesImpl);
    storage_->set_http_server_properties(std::move(server_properties));

    std::unique_ptr<net::MultiLogCTVerifier> ct_verifier =
        base::MakeUnique<net::MultiLogCTVerifier>();
    ct_verifier->AddLogs(net::ct::CreateLogVerifiersForKnownLogs());
    storage_->set_cert_transparency_verifier(std::move(ct_verifier));
    storage_->set_ct_policy_enforcer(base::MakeUnique<net::CTPolicyEnforcer>());

    net::HttpNetworkSession::Params network_session_params;
    network_session_params.ignore_certificate_errors = false;

    // TODO(hferreiro): enable?
    // disable quic until webrequest filtering is added
    // https://github.com/brave/browser-laptop/issues/6831
    network_session_params.enable_quic = false;

    // --disable-http2
    if (command_line.HasSwitch(switches::kDisableHttp2)) {
      network_session_params.enable_http2 = false;
    }

    // --ignore-certificate-errors
    if (command_line.HasSwitch(switches::kIgnoreCertificateErrors))
      network_session_params.ignore_certificate_errors = true;

    // --host-rules
    if (command_line.HasSwitch(switches::kHostRules)) {
      host_mapping_rules_.reset(new net::HostMappingRules);
      host_mapping_rules_->SetRulesFromString(command_line.GetSwitchValueASCII(switches::kHostRules));
      network_session_params.host_mapping_rules = *host_mapping_rules_.get();
    }

    net::HttpNetworkSession::Context network_session_context;
    net::URLRequestContextBuilder::SetHttpNetworkSessionComponents(
        url_request_context_.get(), &network_session_context);

    // Give |storage_| ownership at the end in case it's |mapped_host_resolver|.
    storage_->set_host_resolver(std::move(host_resolver));
    network_session_context.host_resolver = url_request_context_->host_resolver();

    http_network_session_.reset(
        new net::HttpNetworkSession(network_session_params, network_session_context));

    std::unique_ptr<net::HttpCache::BackendFactory> backend;
    if (in_memory_) {
      backend = net::HttpCache::DefaultBackend::InMemory(0);
    } else {
      backend.reset(delegate_->CreateHttpCacheBackendFactory(base_path_));
    }

    storage_->set_http_transaction_factory(base::WrapUnique(
       new net::HttpCache(content::CreateDevToolsNetworkTransactionFactory(
                              http_network_session_.get()),
                          std::move(backend), false)));

    std::unique_ptr<net::URLRequestJobFactory> job_factory =
        delegate_->CreateURLRequestJobFactory(&protocol_handlers_);

    // Set up interceptors in the reverse order.
    std::unique_ptr<net::URLRequestJobFactory> top_job_factory =
        std::move(job_factory);
    content::URLRequestInterceptorScopedVector::reverse_iterator it;
    for (it = protocol_interceptors_.rbegin();
         it != protocol_interceptors_.rend();
         ++it) {
      top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
          std::move(top_job_factory), std::move(*it)));
    }
    protocol_interceptors_.clear();

    storage_->set_job_factory(std::move(top_job_factory));
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner> URLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
}

}  // namespace brightray
