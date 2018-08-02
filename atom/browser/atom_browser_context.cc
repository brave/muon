// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "atom/browser/atom_browser_context.h"

#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_download_manager_delegate.h"
#include "atom/browser/browser.h"
#include "atom/browser/net/asar/asar_protocol_handler.h"
#include "atom/browser/net/atom_cert_verifier.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/browser/net/atom_ssl_config_service.h"
#include "atom/browser/net/http_protocol_handler.h"
#include "atom/common/atom_version.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_version.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_constants.h"
#include "net/ftp/ftp_network_layer.h"
#include "net/net_buildflags.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/ftp_protocol_handler.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "url/url_constants.h"

using content::BrowserThread;
using net::URLRequestJobFactoryImpl;

namespace atom {

namespace {

class NoCacheBackend : public net::HttpCache::BackendFactory {
  int CreateBackend(net::NetLog* net_log,
                    std::unique_ptr<disk_cache::Backend>* backend,
                    const net::CompletionCallback& callback) override {
    return net::ERR_FAILED;
  }
};

}  // namespace

AtomBrowserContext::AtomBrowserContext(
    const std::string& partition, bool in_memory,
    const base::DictionaryValue& options)
    : brightray::BrowserContext(partition, in_memory) {
  // Read options.
  use_cache_ = true;
  options.GetBoolean("cache", &use_cache_);

  // Initialize Pref Registry in brightray.
  // InitPrefs();
}

AtomBrowserContext::~AtomBrowserContext() {
  BrowserThread::DeleteSoon(BrowserThread::UI,
                              FROM_HERE,
                              download_manager_delegate_.release());
}

std::unique_ptr<net::URLRequestJobFactory>
AtomBrowserContext::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* protocol_handlers) {
  std::unique_ptr<URLRequestJobFactoryImpl> job_factory(
      new URLRequestJobFactoryImpl);

  for (auto& it : *protocol_handlers) {
    job_factory->SetProtocolHandler(it.first,
                                    base::WrapUnique(it.second.release()));
  }
  protocol_handlers->clear();

  job_factory->SetProtocolHandler(
      url::kDataScheme, base::WrapUnique(new net::DataProtocolHandler));
  job_factory->SetProtocolHandler(
      url::kFileScheme,
      std::make_unique<net::FileProtocolHandler>(
          base::CreateTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
               base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})));
  job_factory->SetProtocolHandler(
      "brave",
      base::WrapUnique(
          new asar::AsarProtocolHandler(base::CreateTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
               base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}))));
  job_factory->SetProtocolHandler(
      url::kHttpScheme,
      base::WrapUnique(new HttpProtocolHandler(url::kHttpScheme)));
  job_factory->SetProtocolHandler(
      url::kHttpsScheme,
      base::WrapUnique(new HttpProtocolHandler(url::kHttpsScheme)));
  job_factory->SetProtocolHandler(
      url::kWsScheme,
      base::WrapUnique(new HttpProtocolHandler(url::kWsScheme)));
  job_factory->SetProtocolHandler(
      url::kWssScheme,
      base::WrapUnique(new HttpProtocolHandler(url::kWssScheme)));

#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  auto host_resolver =
      url_request_context_getter()->GetURLRequestContext()->host_resolver();
  job_factory->SetProtocolHandler(
      url::kFtpScheme,
      net::FtpProtocolHandler::Create(host_resolver));
#endif  // !BUILDFLAG(DISABLE_FTP_SUPPORT)

  return std::move(job_factory);
}

net::HttpCache::BackendFactory*
AtomBrowserContext::CreateHttpCacheBackendFactory(
    const base::FilePath& base_path) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!use_cache_ || command_line->HasSwitch(switches::kDisableHttpCache))
    return new NoCacheBackend;
  else
    return brightray::BrowserContext::CreateHttpCacheBackendFactory(base_path);
}

content::DownloadManagerDelegate*
AtomBrowserContext::GetDownloadManagerDelegate() {
  if (!download_manager_delegate_.get()) {
    auto download_manager = content::BrowserContext::GetDownloadManager(this);
    download_manager_delegate_.reset(
        new AtomDownloadManagerDelegate(download_manager));
  }
  return download_manager_delegate_.get();
}

std::unique_ptr<net::CertVerifier> AtomBrowserContext::CreateCertVerifier() {
  return base::WrapUnique(new AtomCertVerifier);
}

net::SSLConfigService* AtomBrowserContext::CreateSSLConfigService() {
  return new AtomSSLConfigService;
}

std::vector<std::string> AtomBrowserContext::GetCookieableSchemes() {
  auto default_schemes = brightray::BrowserContext::GetCookieableSchemes();
  const auto& standard_schemes = atom::api::GetStandardSchemes();
  default_schemes.insert(default_schemes.end(),
                         standard_schemes.begin(), standard_schemes.end());
  return default_schemes;
}

void AtomBrowserContext::RegisterPrefs(PrefRegistrySimple* pref_registry) {
  // moved to user_prefs in brave_browser_context
  pref_registry->RegisterFilePathPref(prefs::kDownloadDefaultDirectory,
                                      base::FilePath());
  pref_registry->RegisterDictionaryPref(prefs::kDevToolsFileSystemPaths);
}

}  // namespace atom
