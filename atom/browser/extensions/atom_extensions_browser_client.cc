// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "atom/browser/browser.h"
#include "atom/browser/extensions/api/atom_extensions_api_client.h"
#include "atom/browser/extensions/atom_extension_host_delegate.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"
#include "atom/browser/extensions/atom_extension_web_contents_observer.h"
#include "atom/browser/extensions/atom_extensions_browser_client.h"
#include "atom/browser/extensions/atom_process_manager_delegate.h"
#include "atom/browser/web_contents_preferences.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/task_scheduler/post_task.h"
#include "base/version.h"
#include "brave/browser/brave_browser_context.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/chrome_component_extension_resource_manager.h"
#include "chrome/browser/extensions/chrome_extension_api_frame_id_map_helper.h"
#include "chrome/browser/extensions/event_router_forwarder.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/common/chrome_paths.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/api/alarms/alarms_api.h"
#include "extensions/browser/api/audio/audio_api.h"
#include "extensions/browser/api/declarative/declarative_api.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/api/idle/idle_api.h"
#include "extensions/browser/api/management/management_api.h"
#include "extensions/browser/api/runtime/runtime_api.h"
#include "extensions/browser/api/runtime/runtime_api_delegate.h"
#include "extensions/browser/api/socket/socket_api.h"
#include "extensions/browser/api/sockets_tcp/sockets_tcp_api.h"
#include "extensions/browser/api/sockets_tcp_server/sockets_tcp_server_api.h"
#include "extensions/browser/api/sockets_udp/sockets_udp_api.h"
#include "extensions/browser/api/storage/storage_api.h"
#include "extensions/browser/api/web_request/web_request_api.h"
#include "extensions/browser/component_extension_resource_manager.h"
#include "extensions/browser/extension_function_registry.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_util.h"
#include "extensions/browser/mojo/interface_registration.h"
#include "extensions/browser/pref_names.h"
#include "extensions/browser/updater/null_extension_cache.h"
#include "extensions/browser/url_request_util.h"
#include "extensions/common/features/behavior_feature.h"
#include "extensions/common/features/feature.h"
#include "extensions/common/features/feature_provider.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "extensions/shell/browser/delegates/shell_kiosk_delegate.h"
#include "ui/base/resource/resource_bundle.h"

// URLRequestResourceBundleJob
// TODO(bridiver) move to a separate file
#include "base/base64.h"
#include "base/format_macros.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/extensions/api/cryptotoken_private/cryptotoken_private_api.h"
#include "electron/brave/common/extensions/api/generated_api_registration.h"
#include "extensions/browser/api/generated_api_registration.h"
#include "extensions/common/file_util.h"
#include "net/url_request/url_request_simple_job.h"

namespace {

bool IsWhitelistedForIncognito(const extensions::Extension* extension) {
  const extensions::Feature* feature =
      extensions::FeatureProvider::GetBehaviorFeature(
          extensions::behavior_feature::kWhitelistedForIncognito);
  return feature && feature->IsAvailableToExtension(extension).is_available();
}

net::HttpResponseHeaders* BuildHttpHeaders(
    const std::string& content_security_policy,
    bool send_cors_header,
    const base::Time& last_modified_time) {
  std::string raw_headers;
  raw_headers.append("HTTP/1.1 200 OK");
  if (!content_security_policy.empty()) {
    raw_headers.append(1, '\0');
    raw_headers.append("Content-Security-Policy: ");
    raw_headers.append(content_security_policy);
  }

  if (send_cors_header) {
    raw_headers.append(1, '\0');
    raw_headers.append("Access-Control-Allow-Origin: *");
  }

  if (!last_modified_time.is_null()) {
    // Hash the time and make an etag to avoid exposing the exact
    // user installation time of the extension.
    std::string hash =
        base::StringPrintf("%" PRId64, last_modified_time.ToInternalValue());
    hash = base::SHA1HashString(hash);
    std::string etag;
    base::Base64Encode(hash, &etag);
    raw_headers.append(1, '\0');
    raw_headers.append("ETag: \"");
    raw_headers.append(etag);
    raw_headers.append("\"");
    // Also force revalidation.
    raw_headers.append(1, '\0');
    raw_headers.append("cache-control: no-cache");
  }

  raw_headers.append(2, '\0');
  return new net::HttpResponseHeaders(raw_headers);
}

// A request for an extension resource in a Chrome .pak file. These are used
// by component extensions.
class URLRequestResourceBundleJob : public net::URLRequestSimpleJob {
 public:
  URLRequestResourceBundleJob(net::URLRequest* request,
                              net::NetworkDelegate* network_delegate,
                              const base::FilePath& filename,
                              int resource_id,
                              const std::string& content_security_policy,
                              bool send_cors_header)
      : net::URLRequestSimpleJob(request, network_delegate),
        filename_(filename),
        resource_id_(resource_id),
        weak_factory_(this) {
    // Leave cache headers out of resource bundle requests.
    response_info_.headers = BuildHttpHeaders(
        content_security_policy, send_cors_header, base::Time());
  }

  // Overridden from URLRequestSimpleJob:
  int GetRefCountedData(
      std::string* mime_type,
      std::string* charset,
      scoped_refptr<base::RefCountedMemory>* data,
      const net::CompletionCallback& callback) const override {
    const ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
    *data = rb.LoadDataResourceBytes(resource_id_);

    // Add the Content-Length header now that we know the resource length.
    response_info_.headers->AddHeader(
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kContentLength,
                           base::NumberToString((*data)->size()).c_str()));

    std::string* read_mime_type = new std::string;
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock{}},
        base::Bind(&net::GetMimeTypeFromFile, filename_,
                   base::Unretained(read_mime_type)),
        base::Bind(&URLRequestResourceBundleJob::OnMimeTypeRead,
                   weak_factory_.GetWeakPtr(), mime_type, charset, *data,
                   base::Owned(read_mime_type), callback));

    return net::ERR_IO_PENDING;
  }

  void GetResponseInfo(net::HttpResponseInfo* info) override {
    *info = response_info_;
  }

 private:
  ~URLRequestResourceBundleJob() override {}

  void OnMimeTypeRead(std::string* out_mime_type,
                      std::string* charset,
                      scoped_refptr<base::RefCountedMemory> data,
                      std::string* read_mime_type,
                      const net::CompletionCallback& callback,
                      bool read_result) {
    *out_mime_type = *read_mime_type;
    if (base::StartsWith(*read_mime_type, "text/",
                         base::CompareCase::INSENSITIVE_ASCII)) {
      // All of our HTML files should be UTF-8 and for other resource types
      // (like images), charset doesn't matter.
      DCHECK(base::IsStringUTF8(base::StringPiece(
          reinterpret_cast<const char*>(data->front()), data->size())));
      *charset = "utf-8";
    }
    int result = read_result ? net::OK : net::ERR_INVALID_URL;
    callback.Run(result);
  }

  // We need the filename of the resource to determine the mime type.
  base::FilePath filename_;

  // The resource bundle id to load.
  int resource_id_;

  net::HttpResponseInfo response_info_;

  mutable base::WeakPtrFactory<URLRequestResourceBundleJob> weak_factory_;
};

}  // namespace

namespace extensions {

class AtomRuntimeAPIDelegate : public RuntimeAPIDelegate {
 public:
  AtomRuntimeAPIDelegate() {}
  ~AtomRuntimeAPIDelegate() override {};

  // RuntimeAPIDelegate implementation.
  void AddUpdateObserver(UpdateObserver* observer) override {};
  void RemoveUpdateObserver(UpdateObserver* observer) override {};
  void ReloadExtension(const std::string& extension_id) override {};
  bool CheckForUpdates(const std::string& extension_id,
                       const UpdateCheckCallback& callback) override {
    return false;
  };
  void OpenURL(const GURL& uninstall_url) override {};
  bool GetPlatformInfo(api::runtime::PlatformInfo* info) override {
    return true;
  };
  bool RestartDevice(std::string* error_message) override { return false; };

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomRuntimeAPIDelegate);
};

AtomExtensionsBrowserClient::AtomExtensionsBrowserClient()
  : extension_cache_(new NullExtensionCache()) {
    api_client_.reset(new AtomExtensionsAPIClient);
    process_manager_delegate_.reset(new AtomProcessManagerDelegate);
    resource_manager_.reset(new ChromeComponentExtensionResourceManager);
}

AtomExtensionsBrowserClient::~AtomExtensionsBrowserClient() {}

std::unique_ptr<ExtensionApiFrameIdMapHelper>
AtomExtensionsBrowserClient::CreateExtensionApiFrameIdMapHelper(
    ExtensionApiFrameIdMap* map) {
  return base::WrapUnique(new ChromeExtensionApiFrameIdMapHelper(map));
}

bool AtomExtensionsBrowserClient::IsShuttingDown() {
  return atom::Browser::Get()->is_shutting_down();
}

bool AtomExtensionsBrowserClient::AreExtensionsDisabled(
    const base::CommandLine& command_line,
    content::BrowserContext* context) {
  return false;
}

bool AtomExtensionsBrowserClient::IsValidContext(
    content::BrowserContext* context) {
  return true;
}

bool AtomExtensionsBrowserClient::IsSameContext(
    content::BrowserContext* first,
    content::BrowserContext* second) {
  if (GetOriginalContext(first) == GetOriginalContext(second))
    return true;

  return false;
}

bool AtomExtensionsBrowserClient::HasOffTheRecordContext(
    content::BrowserContext* context) {
  return static_cast<brave::BraveBrowserContext*>(context)->otr_context()
      != nullptr;
}

content::BrowserContext* AtomExtensionsBrowserClient::GetOffTheRecordContext(
    content::BrowserContext* context) {
  return static_cast<brave::BraveBrowserContext*>(context)->otr_context();
}

content::BrowserContext* AtomExtensionsBrowserClient::GetOriginalContext(
    content::BrowserContext* context) {
  return static_cast<brave::BraveBrowserContext*>(context)->original_context();
}

bool AtomExtensionsBrowserClient::IsGuestSession(
    content::BrowserContext* context) const {
  // return true for sub-contexts so the process manager original context
  // will match the extension registry context
  auto original_context =
      static_cast<brave::BraveBrowserContext*>(context)->original_context();
  return !context->IsOffTheRecord() && (context != original_context);
}

// static
bool AtomExtensionsBrowserClient::IsIncognitoEnabled(
      const std::string& extension_id,
      content::BrowserContext* context) {
  auto registry = ExtensionRegistry::Get(context);
  if (!registry)
    return false;

  const Extension* extension = registry->
      GetExtensionById(extension_id, ExtensionRegistry::ENABLED);
  if (extension) {
    if (!util::CanBeIncognitoEnabled(extension))
      return false;
    if (extension->location() == Manifest::COMPONENT ||
        extension->location() == Manifest::EXTERNAL_COMPONENT) {
      return true;
    }
    if (IsWhitelistedForIncognito(extension))
      return true;
  }

  // TODO(bridiver)
  // ExtensionPrefs::Get(context)->IsIncognitoEnabled(extension_id);
  return true;
}

bool AtomExtensionsBrowserClient::CanExtensionCrossIncognito(
    const Extension* extension,
    content::BrowserContext* context) const {
  DCHECK(extension);
  return IsExtensionIncognitoEnabled(extension->id(), context) &&
         !IncognitoInfo::IsSplitMode(extension);
}

net::URLRequestJob*
AtomExtensionsBrowserClient::MaybeCreateResourceBundleRequestJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const base::FilePath& directory_path,
    const std::string& content_security_policy,
    bool send_cors_header) {
  base::FilePath resources_path;
  base::FilePath relative_path;

  PathService::Get(chrome::DIR_RESOURCES, &resources_path);

  if (PathService::Get(chrome::DIR_RESOURCES, &resources_path) &&
      resources_path.AppendRelativePath(directory_path, &relative_path)) {
    base::FilePath request_path =
        extensions::file_util::ExtensionURLToRelativeFilePath(request->url());
    int resource_id = 0;
    if (ExtensionsBrowserClient::Get()
            ->GetComponentExtensionResourceManager()
            ->IsComponentExtensionResource(
                directory_path, request_path, &resource_id)) {
      relative_path = relative_path.Append(request_path);
      relative_path = relative_path.NormalizePathSeparators();

      return new URLRequestResourceBundleJob(request,
                                             network_delegate,
                                             relative_path,
                                             resource_id,
                                             content_security_policy,
                                             send_cors_header);
    }
  }
  return NULL;
}

bool AtomExtensionsBrowserClient::AllowCrossRendererResourceLoad(
    const GURL& url,
    content::ResourceType resource_type,
    ui::PageTransition page_transition,
    int child_id,
    bool is_incognito,
    const Extension* extension,
    const ExtensionSet& extensions,
    const ProcessMap& process_map) {

  bool allowed = false;
  if (url_request_util::AllowCrossRendererResourceLoad(
          url, resource_type, page_transition, child_id, is_incognito,
          extension, extensions, process_map, &allowed)) {
    return allowed;
  }

  // Couldn't determine if resource is allowed. Block the load.
  return false;
}

void AtomExtensionsBrowserClient::GetEarlyExtensionPrefsObservers(
    content::BrowserContext* context,
    std::vector<ExtensionPrefsObserver*>* observers) const {
}

ProcessManagerDelegate*
AtomExtensionsBrowserClient::GetProcessManagerDelegate() const {
  return process_manager_delegate_.get();
}

std::unique_ptr<ExtensionHostDelegate>
AtomExtensionsBrowserClient::CreateExtensionHostDelegate() {
  return std::unique_ptr<ExtensionHostDelegate>(new AtomExtensionHostDelegate);
}

PrefService* AtomExtensionsBrowserClient::GetPrefServiceForContext(
    content::BrowserContext* context) {
  return user_prefs::UserPrefs::Get(context);
}

bool AtomExtensionsBrowserClient::DidVersionUpdate(
    content::BrowserContext* context) {
  return false;
}

void AtomExtensionsBrowserClient::PermitExternalProtocolHandler() {
}

bool AtomExtensionsBrowserClient::IsRunningInForcedAppMode() {
  return false;
}

bool AtomExtensionsBrowserClient::IsLoggedInAsPublicAccount() {
  return false;
}

ExtensionSystemProvider*
AtomExtensionsBrowserClient::GetExtensionSystemFactory() {
  return AtomExtensionSystemFactory::GetInstance();
}

void AtomExtensionsBrowserClient::RegisterExtensionFunctions(
    ExtensionFunctionRegistry* registry) const {
  api::GeneratedFunctionRegistry::RegisterAll(registry);
  api::BraveGeneratedFunctionRegistry::RegisterAll(registry);

  // Instead of registering all Chrome extension functions,
  // via api::ChromeGeneratedFunctionRegistry::RegisterAll(registry)
  // explicitly register just the ones we want
  constexpr ExtensionFunctionRegistry::FactoryEntry chromeEntries[] = {
      {
          &NewExtensionFunction<
              api::CryptotokenPrivateCanOriginAssertAppIdFunction>,
          api::CryptotokenPrivateCanOriginAssertAppIdFunction::function_name(),
          api::CryptotokenPrivateCanOriginAssertAppIdFunction::
              histogram_value(),
      },
  };

  for (const auto& entry : chromeEntries) {
      registry->Register(entry);
  }
}

std::unique_ptr<RuntimeAPIDelegate>
AtomExtensionsBrowserClient::CreateRuntimeAPIDelegate(
    content::BrowserContext* context) const {
  return std::unique_ptr<RuntimeAPIDelegate>(new AtomRuntimeAPIDelegate());
}

const ComponentExtensionResourceManager*
AtomExtensionsBrowserClient::GetComponentExtensionResourceManager() {
  return resource_manager_.get();
}

void AtomExtensionsBrowserClient::RegisterExtensionInterfaces(
    service_manager::BinderRegistryWithArgs<
        content::RenderFrameHost*>* registry,
    content::RenderFrameHost* render_frame_host,
    const Extension* extension) const {
  RegisterInterfacesForExtension(registry, render_frame_host, extension);
}

void AtomExtensionsBrowserClient::BroadcastEventToRenderers(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    std::unique_ptr<base::ListValue> args) {
  g_browser_process->extension_event_router_forwarder()
      ->BroadcastEventToRenderers(histogram_value, event_name, std::move(args),
                                  GURL());
}

net::NetLog* AtomExtensionsBrowserClient::GetNetLog() {
  return nullptr;
}

ExtensionCache* AtomExtensionsBrowserClient::GetExtensionCache() {
  return extension_cache_.get();
}

bool AtomExtensionsBrowserClient::IsBackgroundUpdateAllowed() {
  return true;
}

bool AtomExtensionsBrowserClient::IsMinBrowserVersionSupported(
    const std::string& min_version) {
  return true;
}

ExtensionWebContentsObserver*
AtomExtensionsBrowserClient::GetExtensionWebContentsObserver(
    content::WebContents* web_contents) {
  return AtomExtensionWebContentsObserver::FromWebContents(web_contents);;
}

ExtensionNavigationUIData*
AtomExtensionsBrowserClient::GetExtensionNavigationUIData(
    net::URLRequest* request) {
  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!info)
    return nullptr;
  ChromeNavigationUIData* navigation_data =
      static_cast<ChromeNavigationUIData*>(info->GetNavigationUIData());
  if (!navigation_data)
    return nullptr;
  return navigation_data->GetExtensionNavigationUIData();
}

KioskDelegate* AtomExtensionsBrowserClient::GetKioskDelegate() {
  return nullptr;
}

void AtomExtensionsBrowserClient::CleanUpWebView(
    content::BrowserContext* browser_context,
    int embedder_process_id,
    int view_instance_id) {
}

void AtomExtensionsBrowserClient::AttachExtensionTaskManagerTag(
    content::WebContents* web_contents,
    ViewType view_type) {
}

bool AtomExtensionsBrowserClient::IsLockScreenContext(
    content::BrowserContext* context) {
  return false;
}

std::string AtomExtensionsBrowserClient::GetApplicationLocale() {
  return g_browser_process->GetApplicationLocale();
}

}  // namespace extensions
