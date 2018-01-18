// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_session.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/api/atom_api_autofill.h"
#include "atom/browser/api/atom_api_content_settings.h"
#include "atom/browser/api/atom_api_cookies.h"
#include "atom/browser/api/atom_api_download_item.h"
#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/api/atom_api_spellchecker.h"
#include "atom/browser/api/atom_api_user_prefs.h"
#include "atom/browser/api/atom_api_web_request.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/browser.h"
#include "atom/browser/net/atom_cert_verifier.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "base/files/file_path.h"
#include "base/guid.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/brave_permission_manager.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/history/core/browser/history_service.h"
#include "components/prefs/pref_service.h"
#include "content/network/throttling/network_conditions.h"
#include "content/network/throttling/throttling_controller.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/features/features.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/base/load_flags.h"
#include "net/disk_cache/disk_cache.h"
#include "net/dns/host_cache.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_preferences.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/proxy/proxy_service.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/api/brave_api_extension.h"
#include "extensions/browser/extensions_browser_client.h"
#endif

using content::BrowserThread;
using content::NetworkConditions;
using content::StoragePartition;
using content::ThrottlingController;

namespace {

struct ClearStorageDataOptions {
  GURL origin;
  uint32_t storage_types = StoragePartition::REMOVE_DATA_MASK_ALL;
  uint32_t quota_types = StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL;
};

uint32_t GetStorageMask(const std::vector<std::string>& storage_types) {
  uint32_t storage_mask = 0;
  for (const auto& it : storage_types) {
    auto type = base::ToLowerASCII(it);
    if (type == "appcache")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_APPCACHE;
    else if (type == "cookies")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_COOKIES;
    else if (type == "filesystem")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS;
    else if (type == "indexdb")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_INDEXEDDB;
    else if (type == "localstorage")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE;
    else if (type == "shadercache")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE;
    else if (type == "websql")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_WEBSQL;
    else if (type == "serviceworkers")
      storage_mask |= StoragePartition::REMOVE_DATA_MASK_SERVICE_WORKERS;
  }
  return storage_mask;
}

uint32_t GetQuotaMask(const std::vector<std::string>& quota_types) {
  uint32_t quota_mask = 0;
  for (const auto& it : quota_types) {
    auto type = base::ToLowerASCII(it);
    if (type == "temporary")
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_TEMPORARY;
    else if (type == "persistent")
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_PERSISTENT;
    else if (type == "syncable")
      quota_mask |= StoragePartition::QUOTA_MANAGED_STORAGE_MASK_SYNCABLE;
  }
  return quota_mask;
}

void SetEnableBrotliInIO(scoped_refptr<net::URLRequestContextGetter> getter,
                         bool enabled) {
  getter->GetURLRequestContext()->set_enable_brotli(enabled);
}

}  // namespace

namespace mate {

template<>
struct Converter<ClearStorageDataOptions> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ClearStorageDataOptions* out) {
    mate::Dictionary options;
    if (!ConvertFromV8(isolate, val, &options))
      return false;
    options.Get("origin", &out->origin);
    std::vector<std::string> types;
    if (options.Get("storages", &types))
      out->storage_types = GetStorageMask(types);
    if (options.Get("quotas", &types))
      out->quota_types = GetQuotaMask(types);
    return true;
  }
};

template<>
struct Converter<net::ProxyConfig> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     net::ProxyConfig* out) {
    std::string proxy_rules, proxy_bypass_rules;
    GURL pac_url;
    mate::Dictionary options;
    // Fallback to previous API when passed String.
    // https://git.io/vuhjj
    if (ConvertFromV8(isolate, val, &proxy_rules)) {
      pac_url = GURL(proxy_rules);  // Assume it is PAC script if it is URL.
    } else if (ConvertFromV8(isolate, val, &options)) {
      options.Get("pacScript", &pac_url);
      options.Get("proxyRules", &proxy_rules);
      options.Get("proxyBypassRules", &proxy_bypass_rules);
    } else {
      return false;
    }

    // pacScript takes precedence over proxyRules.
    if (!pac_url.is_empty() && pac_url.is_valid()) {
      out->set_pac_url(pac_url);
    } else {
      out->proxy_rules().ParseFromString(proxy_rules);
      out->proxy_rules().bypass_rules.ParseFromString(proxy_bypass_rules);
    }
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

namespace {

// Referenced session objects.
std::map<uint32_t, v8::Global<v8::Object>> g_sessions;

class ResolveProxyHelper {
 public:
  ResolveProxyHelper(scoped_refptr<net::URLRequestContextGetter> context_getter,
                     const GURL& url,
                     Session::ResolveProxyCallback callback)
      : callback_(callback),
        original_thread_(base::ThreadTaskRunnerHandle::Get()) {
    context_getter->GetNetworkTaskRunner()->PostTask(
        FROM_HERE,
        base::Bind(&ResolveProxyHelper::ResolveProxy,
                   base::Unretained(this), context_getter, url));
  }

  void OnResolveProxyCompleted(int result) {
    std::string proxy;
    if (result == net::OK)
      proxy = proxy_info_.ToPacString();
    original_thread_->PostTask(FROM_HERE,
                               base::Bind(callback_, proxy));
    delete this;
  }

 private:
  void ResolveProxy(scoped_refptr<net::URLRequestContextGetter> context_getter,
                    const GURL& url) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

    net::ProxyService* proxy_service =
        context_getter->GetURLRequestContext()->proxy_service();
    net::CompletionCallback completion_callback =
        base::Bind(&ResolveProxyHelper::OnResolveProxyCompleted,
                   base::Unretained(this));

    // Start the request.
    int result = proxy_service->ResolveProxy(
        url, "GET", &proxy_info_, completion_callback,
        &pac_req_, nullptr, net::NetLogWithSource());

    // Completed synchronously.
    if (result != net::ERR_IO_PENDING)
      completion_callback.Run(result);
  }

  Session::ResolveProxyCallback callback_;
  net::ProxyInfo proxy_info_;
  net::ProxyService::Request* pac_req_;
  scoped_refptr<base::SingleThreadTaskRunner> original_thread_;

  DISALLOW_COPY_AND_ASSIGN(ResolveProxyHelper);
};

// Runs the callback in UI thread.
template<typename ...T>
void RunCallbackInUI(const base::Callback<void(T...)>& callback, T... result) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::Bind(callback, result...));
}

// Callback of HttpCache::GetBackend.
void OnGetBackend(disk_cache::Backend** backend_ptr,
                  Session::CacheAction action,
                  const net::CompletionCallback& callback,
                  int result) {
  if (result != net::OK) {
    RunCallbackInUI(callback, result);
  } else if (backend_ptr && *backend_ptr) {
    if (action == Session::CacheAction::CLEAR) {
      (*backend_ptr)->DoomAllEntries(base::Bind(&RunCallbackInUI<int>,
                                                callback));
    } else if (action == Session::CacheAction::STATS) {
      base::StringPairs stats;
      (*backend_ptr)->GetStats(&stats);
      for (const auto& stat : stats) {
        if (stat.first == "Current size") {
          int current_size;
          base::StringToInt(stat.second, &current_size);
          RunCallbackInUI(callback, current_size);
          break;
        }
      }
    }
  } else {
    RunCallbackInUI<int>(callback, net::ERR_FAILED);
  }
}

void DoCacheActionInIO(
    const scoped_refptr<net::URLRequestContextGetter>& context_getter,
    Session::CacheAction action,
    const net::CompletionCallback& callback) {
  auto request_context = context_getter->GetURLRequestContext();
  auto http_cache = request_context->http_transaction_factory()->GetCache();
  if (!http_cache)
    RunCallbackInUI<int>(callback, net::ERR_FAILED);

  // Call GetBackend and make the backend's ptr accessable in OnGetBackend.
  using BackendPtr = disk_cache::Backend*;
  auto* backend_ptr = new BackendPtr(nullptr);
  net::CompletionCallback on_get_backend =
      base::Bind(&OnGetBackend, base::Owned(backend_ptr), action, callback);
  int rv = http_cache->GetBackend(backend_ptr, on_get_backend);
  if (rv != net::ERR_IO_PENDING)
    on_get_backend.Run(net::OK);
}

void SetProxyInIO(scoped_refptr<net::URLRequestContextGetter> getter,
                  const net::ProxyConfig& config,
                  const base::Closure& callback) {
  auto proxy_service = getter->GetURLRequestContext()->proxy_service();
  proxy_service->ResetConfigService(base::WrapUnique(
      new net::ProxyConfigServiceFixed(config)));
  // Refetches and applies the new pac script if provided.
  proxy_service->ForceReloadProxyConfig();
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE, base::Bind(&base::DoNothing));
}

void SetCertVerifyProcInIO(
    const scoped_refptr<net::URLRequestContextGetter>& context_getter,
    const AtomCertVerifier::VerifyProc& proc) {
  auto request_context = context_getter->GetURLRequestContext();
  static_cast<AtomCertVerifier*>(request_context->cert_verifier())->
      SetVerifyProc(proc);
}

void ClearHostResolverCacheInIO(
    const scoped_refptr<net::URLRequestContextGetter>& context_getter,
    const base::Closure& callback) {
  auto request_context = context_getter->GetURLRequestContext();
  auto cache = request_context->host_resolver()->GetHostCache();
  if (cache) {
    cache->clear();
    DCHECK_EQ(0u, cache->size());
    if (!callback.is_null())
      BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                              base::Bind(&base::DoNothing));
  }
}

void AllowNTLMCredentialsForDomainsInIO(
    const scoped_refptr<net::URLRequestContextGetter>& context_getter,
    const std::string& domains) {
  auto request_context = context_getter->GetURLRequestContext();
  auto auth_handler = request_context->http_auth_handler_factory();
  if (auth_handler) {
    auto auth_preferences = const_cast<net::HttpAuthPreferences*>(
        auth_handler->http_auth_preferences());
    if (auth_preferences)
      auth_preferences->SetServerWhitelist(domains);
  }
}

void OnClearStorageDataDone(const base::Closure& callback) {
  if (!callback.is_null())
    callback.Run();
}

void OnClearHistory() {}

}  // namespace

Session::Session(v8::Isolate* isolate, Profile* profile)
    : devtools_network_emulation_client_id_(base::GenerateGUID()),
      profile_(profile),
      request_context_getter_(profile->GetRequestContext()) {
  // Observe DownloadManger to get download notifications.
  content::BrowserContext::GetDownloadManager(profile)->
      AddObserver(this);

  auto user_prefs_registrar = profile_->user_prefs_change_registrar();
  if (!user_prefs_registrar->IsObserved(prefs::kDownloadDefaultDirectory)) {
    user_prefs_registrar->Add(
        prefs::kDownloadDefaultDirectory,
        base::Bind(&Session::DefaultDownloadDirectoryChanged,
                   base::Unretained(this)));
  }

  Init(isolate);
  AttachAsUserData(profile);
}

Session::~Session() {
  content::BrowserContext::GetDownloadManager(profile_)->
      RemoveObserver(this);
  g_sessions.erase(weak_map_id());
}

std::string Session::Partition() {
  return static_cast<brave::BraveBrowserContext*>(
      profile_)->partition_with_prefix();
}

void Session::DefaultDownloadDirectoryChanged() {
  base::FilePath default_download_path(profile_->GetPrefs()->GetFilePath(
      prefs::kDownloadDefaultDirectory));
  Emit("default-download-directory-changed", default_download_path);
}

void Session::OnDownloadCreated(content::DownloadManager* manager,
                                content::DownloadItem* item) {
  if (item->IsSavePackageDownload())
    return;

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  bool prevent_default = Emit(
      "will-download",
      DownloadItem::Create(isolate(), item),
      item->GetWebContents());
  if (prevent_default) {
    item->Cancel(true);
    item->Remove();
  }
}

void Session::ResolveProxy(const GURL& url, ResolveProxyCallback callback) {
  new ResolveProxyHelper(request_context_getter_, url, callback);
}

template<Session::CacheAction action>
void Session::DoCacheAction(const net::CompletionCallback& callback) {
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&DoCacheActionInIO,
                 request_context_getter_,
                 action,
                 callback));
}

void Session::ClearStorageData(mate::Arguments* args) {
  // clearStorageData([options, callback])
  ClearStorageDataOptions options;
  base::Closure callback;
  args->GetNext(&options);
  args->GetNext(&callback);

  auto storage_partition =
      content::BrowserContext::GetStoragePartition(profile_, nullptr);
  storage_partition->ClearData(
      options.storage_types, options.quota_types, options.origin,
      content::StoragePartition::OriginMatcherFunction(),
      base::Time(), base::Time::Max(),
      base::Bind(&OnClearStorageDataDone, callback));
}

void Session::ClearHistory(mate::Arguments* args) {
  base::Closure callback;
  if (!args->GetNext(&callback))
    callback = base::Bind(&OnClearHistory);

  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfile(
          profile_,
          ServiceAccessType::EXPLICIT_ACCESS);

  history_service->ExpireHistoryBetween(std::set<GURL>(),
                                        base::Time(),
                                        base::Time::Max(),
                                        callback,
                                        &task_tracker_);
}

void Session::FlushStorageData() {
  auto storage_partition =
      content::BrowserContext::GetStoragePartition(profile_, nullptr);
  storage_partition->Flush();
}

void Session::SetProxy(const net::ProxyConfig& config,
                       const base::Closure& callback) {
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&SetProxyInIO, request_context_getter_, config, callback));
}

void Session::SetDownloadPath(const base::FilePath& path) {
  profile_->GetPrefs()->SetFilePath(
      prefs::kDownloadDefaultDirectory, path);
}

void Session::SetCertVerifyProc(v8::Local<v8::Value> val,
                                mate::Arguments* args) {
  AtomCertVerifier::VerifyProc proc;
  if (!(val->IsNull() || mate::ConvertFromV8(args->isolate(), val, &proc))) {
    args->ThrowError("Must pass null or function");
    return;
  }

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&SetCertVerifyProcInIO,
                 request_context_getter_,
                 proc));
}

void Session::SetPermissionRequestHandler(v8::Local<v8::Value> val,
                                          mate::Arguments* args) {
  brave::BravePermissionManager::RequestHandler handler;
  if (!(val->IsNull() || mate::ConvertFromV8(args->isolate(), val, &handler))) {
    args->ThrowError("Must pass null or function");
    return;
  }
  auto permission_manager = static_cast<brave::BravePermissionManager*>(
      profile_->GetPermissionManager());
  permission_manager->SetPermissionRequestHandler(handler);
}

void Session::ClearHostResolverCache(mate::Arguments* args) {
  base::Closure callback;
  args->GetNext(&callback);

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&ClearHostResolverCacheInIO,
                 request_context_getter_,
                 callback));
}

void Session::AllowNTLMCredentialsForDomains(const std::string& domains) {
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&AllowNTLMCredentialsForDomainsInIO,
                 request_context_getter_,
                 domains));
}

void Session::SetEnableBrotli(bool enabled) {
  request_context_getter_->GetNetworkTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&SetEnableBrotliInIO, request_context_getter_, enabled));
}

v8::Local<v8::Value> Session::Cookies(v8::Isolate* isolate) {
  if (cookies_.IsEmpty()) {
    auto handle = atom::api::Cookies::Create(isolate, profile_);
    cookies_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, cookies_);
}

v8::Local<v8::Value> Session::Protocol(v8::Isolate* isolate) {
  if (protocol_.IsEmpty()) {
    auto handle = atom::api::Protocol::Create(isolate, profile_);
    protocol_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, protocol_);
}

v8::Local<v8::Value> Session::WebRequest(v8::Isolate* isolate) {
  if (web_request_.IsEmpty()) {
    auto handle = atom::api::WebRequest::Create(isolate, profile_);
    web_request_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, web_request_);
}

v8::Local<v8::Value> Session::UserPrefs(v8::Isolate* isolate) {
  if (user_prefs_.IsEmpty()) {
    auto handle = atom::api::UserPrefs::Create(isolate, profile_);
    user_prefs_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, user_prefs_);
}

v8::Local<v8::Value> Session::ContentSettings(v8::Isolate* isolate) {
  if (content_settings_.IsEmpty()) {
    auto handle =
        atom::api::ContentSettings::Create(isolate, profile_);
    content_settings_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, content_settings_);
}

v8::Local<v8::Value> Session::Autofill(v8::Isolate* isolate) {
  if (autofill_.IsEmpty()) {
    auto handle = atom::api::Autofill::Create(isolate, profile_);
    autofill_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, autofill_);
}

v8::Local<v8::Value> Session::SpellChecker(v8::Isolate* isolate) {
  if (spell_checker_.IsEmpty()) {
    auto handle = atom::api::SpellChecker::Create(isolate, profile_);
    spell_checker_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, spell_checker_);
}

v8::Local<v8::Value> Session::Extensions(v8::Isolate* isolate) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (extensions_.IsEmpty()) {
    auto handle = brave::api::Extension::Create(isolate, profile_);
    extensions_.Reset(isolate, handle.ToV8());
  }
#endif
  return v8::Local<v8::Value>::New(isolate, extensions_);
}

bool Session::Equal(Session* session) const {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  return extensions::ExtensionsBrowserClient::Get()->IsSameContext(
                                        profile_,
                                        session->browser_context());
#else
  return profile_ == session->browser_context();
#endif
}

// static
mate::Handle<Session> Session::CreateFrom(
    v8::Isolate* isolate, content::BrowserContext* browser_context) {
  auto existing = TrackableObject::FromWrappedClass(isolate, browser_context);
  if (existing)
    return mate::CreateHandle(isolate, static_cast<Session*>(existing));

  auto handle = mate::CreateHandle(
      isolate,
      new Session(isolate, Profile::FromBrowserContext(browser_context)));

  // The Sessions should never be garbage collected, since the common pattern is
  // to use partition strings, instead of using the Session object directly.
  g_sessions[handle->weak_map_id()] =
      v8::Global<v8::Object>(isolate, handle.ToV8());

  return handle;
}

// static
mate::Handle<Session> Session::FromPartition(
    v8::Isolate* isolate, const std::string& partition,
    const base::DictionaryValue& options) {
  AtomBrowserContext* browser_context =
      brave::BraveBrowserContext::FromPartition(partition, options);

  DCHECK(browser_context);
  // TODO(bridiver) - this is a huge hack to deal with sync call
  base::ThreadRestrictions::ScopedAllowWait wait_allowed;
  static_cast<brave::BraveBrowserContext*>(
      browser_context)->ready()->Wait();

  return CreateFrom(isolate, browser_context);
}

// static
void Session::BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Session"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("resolveProxy", &Session::ResolveProxy)
      .SetMethod("getCacheSize", &Session::DoCacheAction<CacheAction::STATS>)
      .SetMethod("clearCache", &Session::DoCacheAction<CacheAction::CLEAR>)
      .SetMethod("clearStorageData", &Session::ClearStorageData)
      .SetMethod("clearHistory", &Session::ClearHistory)
      .SetMethod("flushStorageData", &Session::FlushStorageData)
      .SetMethod("setProxy", &Session::SetProxy)
      .SetMethod("setDownloadPath", &Session::SetDownloadPath)
      .SetMethod("setCertificateVerifyProc", &Session::SetCertVerifyProc)
      .SetMethod("setPermissionRequestHandler",
                 &Session::SetPermissionRequestHandler)
      .SetMethod("clearHostResolverCache", &Session::ClearHostResolverCache)
      .SetMethod("allowNTLMCredentialsForDomains",
                 &Session::AllowNTLMCredentialsForDomains)
      .SetMethod("setEnableBrotli", &Session::SetEnableBrotli)
      .SetMethod("equal", &Session::Equal)
      .SetProperty("partition", &Session::Partition)
      .SetProperty("contentSettings", &Session::ContentSettings)
      .SetProperty("userPrefs", &Session::UserPrefs)
      .SetProperty("cookies", &Session::Cookies)
      .SetProperty("protocol", &Session::Protocol)
      .SetProperty("webRequest", &Session::WebRequest)
      .SetProperty("extensions", &Session::Extensions)
      .SetProperty("autofill", &Session::Autofill)
      .SetProperty("spellChecker", &Session::SpellChecker);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::Session;

v8::Local<v8::Value> FromPartition(
    const std::string& partition, mate::Arguments* args) {
  if (!atom::Browser::Get()->is_ready()) {
    args->ThrowError("Session can only be received when app is ready");
    return v8::Null(args->isolate());
  }
  base::DictionaryValue options;
  args->GetNext(&options);
  return Session::FromPartition(args->isolate(), partition, options).ToV8();
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("Session", Session::GetConstructor(isolate)->GetFunction());
  dict.SetMethod("fromPartition", &FromPartition);
  dict.SetMethod("getAllSessions",
                           &mate::TrackableObject<Session>::GetAll);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_session, Initialize)
