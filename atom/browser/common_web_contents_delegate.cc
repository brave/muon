// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/common_web_contents_delegate.h"

#include <set>
#include <string>
#include <vector>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_security_state_model_client.h"
#include "atom/browser/browser.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_dialog_helper.h"
#include "atom/common/atom_constants.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_javascript_dialog_manager.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/custom_handlers/protocol_handler.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/security_style_explanation.h"
#include "content/public/browser/security_style_explanations.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "net/ssl/ssl_cipher_suite_names.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(ENABLE_EXTENSIONS)
#include "atom/browser/api/atom_api_window.h"
#include "atom/browser/extensions/tab_helper.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "extensions/browser/api/extensions_api_client.h"
#endif

using content::BrowserThread;
using security_state::SecurityStateModel;

namespace atom {

namespace {

const char kRootName[] = "<root>";

struct FileSystem {
  FileSystem() {
  }
  FileSystem(const std::string& file_system_name,
             const std::string& root_url,
             const std::string& file_system_path)
    : file_system_name(file_system_name),
      root_url(root_url),
      file_system_path(file_system_path) {
  }

  std::string file_system_name;
  std::string root_url;
  std::string file_system_path;
};

std::string RegisterFileSystem(content::WebContents* web_contents,
                               const base::FilePath& path) {
  auto isolated_context = storage::IsolatedContext::GetInstance();
  std::string root_name(kRootName);
  std::string file_system_id = isolated_context->RegisterFileSystemForPath(
      storage::kFileSystemTypeNativeLocal,
      std::string(),
      path,
      &root_name);

  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  content::RenderViewHost* render_view_host = web_contents->GetRenderViewHost();
  int renderer_id = render_view_host->GetProcess()->GetID();
  policy->GrantReadFileSystem(renderer_id, file_system_id);
  policy->GrantWriteFileSystem(renderer_id, file_system_id);
  policy->GrantCreateFileForFileSystem(renderer_id, file_system_id);
  policy->GrantDeleteFromFileSystem(renderer_id, file_system_id);

  if (!policy->CanReadFile(renderer_id, path))
    policy->GrantReadFile(renderer_id, path);

  return file_system_id;
}

FileSystem CreateFileSystemStruct(
    content::WebContents* web_contents,
    const std::string& file_system_id,
    const std::string& file_system_path) {
  const GURL origin = web_contents->GetURL().GetOrigin();
  std::string file_system_name =
      storage::GetIsolatedFileSystemName(origin, file_system_id);
  std::string root_url = storage::GetIsolatedFileSystemRootURIString(
      origin, file_system_id, kRootName);
  return FileSystem(file_system_name, root_url, file_system_path);
}

base::DictionaryValue* CreateFileSystemValue(const FileSystem& file_system) {
  auto* file_system_value = new base::DictionaryValue();
  file_system_value->SetString("fileSystemName", file_system.file_system_name);
  file_system_value->SetString("rootURL", file_system.root_url);
  file_system_value->SetString("fileSystemPath", file_system.file_system_path);
  return file_system_value;
}

void WriteToFile(const base::FilePath& path,
                 const std::string& content) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  DCHECK(!path.empty());

  base::WriteFile(path, content.data(), content.size());
}

void AppendToFile(const base::FilePath& path,
                  const std::string& content) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  DCHECK(!path.empty());

  base::AppendToFile(path, content.data(), content.size());
}

PrefService* GetPrefService(content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  return static_cast<atom::AtomBrowserContext*>(context)->prefs();
}

std::set<std::string> GetAddedFileSystemPaths(
    content::WebContents* web_contents) {
  auto pref_service = GetPrefService(web_contents);
  const base::DictionaryValue* file_system_paths_value =
      pref_service->GetDictionary(prefs::kDevToolsFileSystemPaths);
  std::set<std::string> result;
  if (file_system_paths_value) {
    base::DictionaryValue::Iterator it(*file_system_paths_value);
    for (; !it.IsAtEnd(); it.Advance()) {
      result.insert(it.key());
    }
  }
  return result;
}

bool IsDevToolsFileSystemAdded(
    content::WebContents* web_contents,
    const std::string& file_system_path) {
  auto file_system_paths = GetAddedFileSystemPaths(web_contents);
  return file_system_paths.find(file_system_path) != file_system_paths.end();
}

content::SecurityStyle SecurityLevelToSecurityStyle(
    SecurityStateModel::SecurityLevel security_level) {
  switch (security_level) {
    case SecurityStateModel::NONE:
      return content::SECURITY_STYLE_UNAUTHENTICATED;
    case SecurityStateModel::SECURITY_WARNING:
    case SecurityStateModel::SECURITY_POLICY_WARNING:
      return content::SECURITY_STYLE_WARNING;
    case SecurityStateModel::EV_SECURE:
    case SecurityStateModel::SECURE:
      return content::SECURITY_STYLE_AUTHENTICATED;
    case SecurityStateModel::SECURITY_ERROR:
      return content::SECURITY_STYLE_AUTHENTICATION_BROKEN;
  }

  return content::SECURITY_STYLE_UNKNOWN;
}

void AddConnectionExplanation(
    const security_state::SecurityStateModel::SecurityInfo& security_info,
    content::SecurityStyleExplanations* security_style_explanations) {

  // Avoid showing TLS details when we couldn't even establish a TLS connection
  // (e.g. for net errors) or if there was no real connection (some tests). We
  // check the |cert_id| to see if there was a connection.
  if (security_info.cert_id == 0 || security_info.connection_status == 0) {
    return;
  }

  int ssl_version =
      net::SSLConnectionStatusToVersion(security_info.connection_status);
  const char* protocol;
  net::SSLVersionToString(&protocol, ssl_version);
  const char* key_exchange;
  const char* cipher;
  const char* mac;
  bool is_aead;
  uint16_t cipher_suite =
      net::SSLConnectionStatusToCipherSuite(security_info.connection_status);
  net::SSLCipherSuiteToStrings(&key_exchange, &cipher, &mac, &is_aead,
                               cipher_suite);
  base::string16 protocol_name = base::ASCIIToUTF16(protocol);
  base::string16 key_exchange_name = base::ASCIIToUTF16(key_exchange);
  const base::string16 cipher_name =
      (mac == NULL) ? base::ASCIIToUTF16(cipher)
                    : l10n_util::GetStringFUTF16(IDS_CIPHER_WITH_MAC,
                                                 base::ASCIIToUTF16(cipher),
                                                 base::ASCIIToUTF16(mac));
  if (security_info.obsolete_ssl_status == net::OBSOLETE_SSL_NONE) {
    security_style_explanations->secure_explanations.push_back(
        content::SecurityStyleExplanation(
            l10n_util::GetStringUTF8(IDS_STRONG_SSL_SUMMARY),
            l10n_util::GetStringFUTF8(IDS_STRONG_SSL_DESCRIPTION, protocol_name,
                                      key_exchange_name, cipher_name)));
    return;
  }

  std::vector<base::string16> description_replacements;
  int status = security_info.obsolete_ssl_status;
  int str_id;

  str_id = (status & net::OBSOLETE_SSL_MASK_PROTOCOL)
               ? IDS_SSL_AN_OBSOLETE_PROTOCOL
               : IDS_SSL_A_STRONG_PROTOCOL;
  description_replacements.push_back(l10n_util::GetStringUTF16(str_id));
  description_replacements.push_back(protocol_name);

  str_id = (status & net::OBSOLETE_SSL_MASK_KEY_EXCHANGE)
               ? IDS_SSL_AN_OBSOLETE_KEY_EXCHANGE
               : IDS_SSL_A_STRONG_KEY_EXCHANGE;
  description_replacements.push_back(l10n_util::GetStringUTF16(str_id));
  description_replacements.push_back(key_exchange_name);

  str_id = (status & net::OBSOLETE_SSL_MASK_CIPHER) ? IDS_SSL_AN_OBSOLETE_CIPHER
                                                    : IDS_SSL_A_STRONG_CIPHER;
  description_replacements.push_back(l10n_util::GetStringUTF16(str_id));
  description_replacements.push_back(cipher_name);

  security_style_explanations->info_explanations.push_back(
      content::SecurityStyleExplanation(
          l10n_util::GetStringUTF8(IDS_OBSOLETE_SSL_SUMMARY),
          base::UTF16ToUTF8(
              l10n_util::GetStringFUTF16(IDS_OBSOLETE_SSL_DESCRIPTION,
                                         description_replacements, nullptr))));
}

}  // namespace

CommonWebContentsDelegate::CommonWebContentsDelegate()
    : html_fullscreen_(false),
      native_fullscreen_(false),
      devtools_file_system_indexer_(new DevToolsFileSystemIndexer) {
}

CommonWebContentsDelegate::~CommonWebContentsDelegate() {
}

void CommonWebContentsDelegate::InitWithWebContents(
    content::WebContents* web_contents,
    AtomBrowserContext* browser_context) {
  browser_context_ = browser_context;
  web_contents->SetDelegate(this);

  printing::PrintViewManagerBasic::CreateForWebContents(web_contents);
  printing::PrintPreviewMessageHandler::CreateForWebContents(web_contents);

  // Create InspectableWebContents.
  web_contents_.reset(brightray::InspectableWebContents::Create(web_contents));
  web_contents_->SetDelegate(this);

#if defined(ENABLE_EXTENSIONS)
  extensions::ExtensionsAPIClient::Get()->
      AttachWebContentsHelpers(web_contents);
#endif
}

void OnRegisterProtocol(AtomBrowserContext* browser_context,
    const ProtocolHandler &handler,
    bool allowed) {
  ProtocolHandlerRegistry* registry =
    ProtocolHandlerRegistryFactory::GetForBrowserContext(browser_context);
  if (allowed) {
    // Ensure the app is invoked in the first place
    if (!Browser::Get()->
        IsDefaultProtocolClient(handler.protocol(), nullptr)) {
      Browser::Get()->SetAsDefaultProtocolClient(
          handler.protocol(), nullptr);
    }
    registry->OnAcceptRegisterProtocolHandler(handler);
  } else {
    registry->OnDenyRegisterProtocolHandler(handler);
  }
}

// Register a new handler for URL requests with the given scheme.
// |user_gesture| is true if the registration is made in the context of a user
// gesture.
void CommonWebContentsDelegate::RegisterProtocolHandler(
    content::WebContents* web_contents,
  const std::string& protocol,
  const GURL& url,
  bool user_gesture) {
  content::BrowserContext* context = web_contents->GetBrowserContext();
  if (context->IsOffTheRecord())
      return;

  ProtocolHandler handler =
      ProtocolHandler::CreateProtocolHandler(protocol, url);

  ProtocolHandlerRegistry* registry =
      ProtocolHandlerRegistryFactory::GetForBrowserContext(
          browser_context_.get());
  if (registry->IsRegistered(handler))
      return;

  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper)
      return;

  AtomBrowserContext* browser_context =
      static_cast<AtomBrowserContext*>(browser_context_.get());
  auto callback = base::Bind(&OnRegisterProtocol, browser_context, handler);
  permission_helper->RequestProtocolRegistrationPermission(callback,
      user_gesture);
}

// Unregister the registered handler for URL requests with the given scheme.
// |user_gesture| is true if the registration is made in the context of a user
// gesture.
void CommonWebContentsDelegate::UnregisterProtocolHandler(
    content::WebContents* web_contents,
    const std::string& protocol,
    const GURL& url,
    bool user_gesture) {
  if (Browser::Get()->IsDefaultProtocolClient(protocol, nullptr)) {
    Browser::Get()->RemoveAsDefaultProtocolClient(protocol, nullptr);
  }
  ProtocolHandler handler =
      ProtocolHandler::CreateProtocolHandler(protocol, url);
  ProtocolHandlerRegistry* registry =
      ProtocolHandlerRegistryFactory::GetForBrowserContext(
          browser_context_.get());
  registry->RemoveHandler(handler);
}

void CommonWebContentsDelegate::SetOwnerWindow(NativeWindow* owner_window) {
  SetOwnerWindow(GetWebContents(), owner_window);
}

void CommonWebContentsDelegate::SetOwnerWindow(
    content::WebContents* web_contents, NativeWindow* owner_window) {
  owner_window_ = owner_window->GetWeakPtr();
  NativeWindowRelay* relay = new NativeWindowRelay(owner_window_);
  web_contents->SetUserData(relay->key, relay);
#if defined(ENABLE_EXTENSIONS)
  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents);
  if (!tab_helper)
    return;

  int32_t id =
      api::Window::TrackableObject::GetIDFromWrappedClass(owner_window);
  if (id > 0) {
    tab_helper->SetWindowId(id);

    content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_TAB_PARENTED,
      content::Source<content::WebContents>(web_contents),
      content::NotificationService::NoDetails());
  }
#endif
}

void CommonWebContentsDelegate::DestroyWebContents() {
  web_contents_.reset();
}

content::WebContents* CommonWebContentsDelegate::GetWebContents() const {
  if (!web_contents_)
    return nullptr;
  return web_contents_->GetWebContents();
}

content::WebContents*
CommonWebContentsDelegate::GetDevToolsWebContents() const {
  if (!web_contents_)
    return nullptr;
  return web_contents_->GetDevToolsWebContents();
}

content::WebContents* CommonWebContentsDelegate::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  content::NavigationController::LoadURLParams load_url_params(params.url);
  load_url_params.source_site_instance = params.source_site_instance;
  load_url_params.referrer = params.referrer;
  load_url_params.frame_tree_node_id = params.frame_tree_node_id;
  load_url_params.redirect_chain = params.redirect_chain;
  load_url_params.transition_type = params.transition;
  load_url_params.extra_headers = params.extra_headers;
  load_url_params.should_replace_current_entry =
    params.should_replace_current_entry;
  load_url_params.is_renderer_initiated = params.is_renderer_initiated;

  if (params.uses_post) {
    load_url_params.load_type =
      content::NavigationController::LOAD_TYPE_HTTP_POST;
    load_url_params.post_data =
      params.post_data;
  }

  source->GetController().LoadURLWithParams(load_url_params);
  return source;
}

bool CommonWebContentsDelegate::CanOverscrollContent() const {
  return false;
}

content::JavaScriptDialogManager*
CommonWebContentsDelegate::GetJavaScriptDialogManager(
    content::WebContents* source) {
  return brave::BraveJavaScriptDialogManager::GetInstance();
}

content::ColorChooser* CommonWebContentsDelegate::OpenColorChooser(
    content::WebContents* web_contents,
    SkColor color,
    const std::vector<content::ColorSuggestion>& suggestions) {
  return chrome::ShowColorChooser(web_contents, color);
}

void CommonWebContentsDelegate::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    const content::FileChooserParams& params) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(owner_window()));
  web_dialog_helper_->RunFileChooser(render_frame_host, params);
}

void CommonWebContentsDelegate::EnumerateDirectory(content::WebContents* guest,
                                                   int request_id,
                                                   const base::FilePath& path) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(owner_window()));
  web_dialog_helper_->EnumerateDirectory(guest, request_id, path);
}

void CommonWebContentsDelegate::EnterFullscreenModeForTab(
    content::WebContents* source, const GURL& origin) {
  if (!owner_window_)
    return;
  SetHtmlApiFullscreen(true);
  owner_window_->NotifyWindowEnterHtmlFullScreen();
  source->GetRenderViewHost()->GetWidget()->WasResized();
}

void CommonWebContentsDelegate::ExitFullscreenModeForTab(
    content::WebContents* source) {
  if (!owner_window_)
    return;
  SetHtmlApiFullscreen(false);
  owner_window_->NotifyWindowLeaveHtmlFullScreen();
  source->GetRenderViewHost()->GetWidget()->WasResized();
}

bool CommonWebContentsDelegate::IsFullscreenForTabOrPending(
    const content::WebContents* source) const {
  return html_fullscreen_;
}

content::SecurityStyle CommonWebContentsDelegate::GetSecurityStyle(
    content::WebContents* web_contents,
    content::SecurityStyleExplanations* security_style_explanations) {
  auto model_client =
      AtomSecurityStateModelClient::FromWebContents(web_contents);
  auto security_info = model_client->GetSecurityInfo();

  const content::SecurityStyle security_style =
      SecurityLevelToSecurityStyle(security_info.security_level);

  security_style_explanations->ran_insecure_content_style =
      SecurityLevelToSecurityStyle(
          SecurityStateModel::kRanInsecureContentLevel);
  security_style_explanations->displayed_insecure_content_style =
      SecurityLevelToSecurityStyle(
          SecurityStateModel::kDisplayedInsecureContentLevel);

  // Check if the page is HTTP; if so, no explanations are needed. Note
  // that SECURITY_STYLE_UNAUTHENTICATED does not necessarily mean that
  // the page is loaded over HTTP, because the security style merely
  // represents how the embedder wishes to display the security state of
  // the page, and the embedder can choose to display HTTPS page as HTTP
  // if it wants to (for example, displaying deprecated crypto
  // algorithms with the same UI treatment as HTTP pages).
  security_style_explanations->scheme_is_cryptographic =
      security_info.scheme_is_cryptographic;
  if (!security_info.scheme_is_cryptographic) {
    return security_style;
  }

  if (security_info.sha1_deprecation_status ==
      SecurityStateModel::DEPRECATED_SHA1_MAJOR) {
    security_style_explanations->broken_explanations.push_back(
        content::SecurityStyleExplanation(
            l10n_util::GetStringUTF8(IDS_MAJOR_SHA1),
            l10n_util::GetStringUTF8(IDS_MAJOR_SHA1_DESCRIPTION),
            security_info.cert_id));
  } else if (security_info.sha1_deprecation_status ==
             SecurityStateModel::DEPRECATED_SHA1_MINOR) {
    security_style_explanations->unauthenticated_explanations.push_back(
        content::SecurityStyleExplanation(
            l10n_util::GetStringUTF8(IDS_MINOR_SHA1),
            l10n_util::GetStringUTF8(IDS_MINOR_SHA1_DESCRIPTION),
            security_info.cert_id));
  }

  security_style_explanations->ran_insecure_content =
      security_info.mixed_content_status ==
          SecurityStateModel::CONTENT_STATUS_RAN ||
      security_info.mixed_content_status ==
          SecurityStateModel::CONTENT_STATUS_DISPLAYED_AND_RAN;
  security_style_explanations->displayed_insecure_content =
      security_info.mixed_content_status ==
          SecurityStateModel::CONTENT_STATUS_DISPLAYED ||
      security_info.mixed_content_status ==
          SecurityStateModel::CONTENT_STATUS_DISPLAYED_AND_RAN;

  if (net::IsCertStatusError(security_info.cert_status)) {
    base::string16 error_string = base::UTF8ToUTF16(net::ErrorToString(
        net::MapCertStatusToNetError(security_info.cert_status)));

    content::SecurityStyleExplanation explanation(
        l10n_util::GetStringUTF8(IDS_CERTIFICATE_CHAIN_ERROR),
        l10n_util::GetStringFUTF8(
            IDS_CERTIFICATE_CHAIN_ERROR_DESCRIPTION_FORMAT, error_string),
        security_info.cert_id);

    if (net::IsCertStatusMinorError(security_info.cert_status))
      security_style_explanations->unauthenticated_explanations.push_back(
          explanation);
    else
      security_style_explanations->broken_explanations.push_back(explanation);
  } else {
    // If the certificate does not have errors and is not using
    // deprecated SHA1, then add an explanation that the certificate is
    // valid.
    if (security_info.sha1_deprecation_status ==
        SecurityStateModel::NO_DEPRECATED_SHA1) {
      security_style_explanations->secure_explanations.push_back(
          content::SecurityStyleExplanation(
              l10n_util::GetStringUTF8(IDS_VALID_SERVER_CERTIFICATE),
              l10n_util::GetStringUTF8(
                  IDS_VALID_SERVER_CERTIFICATE_DESCRIPTION),
              security_info.cert_id));
    }
  }

  AddConnectionExplanation(security_info, security_style_explanations);

  security_style_explanations->pkp_bypassed = security_info.pkp_bypassed;
  if (security_info.pkp_bypassed) {
    security_style_explanations->info_explanations.push_back(
        content::SecurityStyleExplanation(
            "Public-Key Pinning Bypassed",
            "Public-key pinning was bypassed by a local root certificate."));
  }

  return security_style;
}

void CommonWebContentsDelegate::DevToolsSaveToFile(
    const std::string& url, const std::string& content, bool save_as) {
  base::FilePath path;
  auto it = saved_files_.find(url);
  if (it != saved_files_.end() && !save_as) {
    path = it->second;
  } else {
    file_dialog::Filters filters;
    base::FilePath default_path(base::FilePath::FromUTF8Unsafe(url));
    if (!file_dialog::ShowSaveDialog(owner_window(), url, "", default_path,
                                     filters, &path)) {
      base::StringValue url_value(url);
      web_contents_->CallClientFunction(
          "DevToolsAPI.canceledSaveURL", &url_value, nullptr, nullptr);
      return;
    }
  }

  saved_files_[url] = path;
  BrowserThread::PostTaskAndReply(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&WriteToFile, path, content),
      base::Bind(&CommonWebContentsDelegate::OnDevToolsSaveToFile,
                 base::Unretained(this), url));
}

void CommonWebContentsDelegate::DevToolsAppendToFile(
    const std::string& url, const std::string& content) {
  auto it = saved_files_.find(url);
  if (it == saved_files_.end())
    return;

  BrowserThread::PostTaskAndReply(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&AppendToFile, it->second, content),
      base::Bind(&CommonWebContentsDelegate::OnDevToolsAppendToFile,
                 base::Unretained(this), url));
}

void CommonWebContentsDelegate::DevToolsRequestFileSystems() {
  auto file_system_paths = GetAddedFileSystemPaths(GetDevToolsWebContents());
  if (file_system_paths.empty()) {
    base::ListValue empty_file_system_value;
    web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                      &empty_file_system_value,
                                      nullptr, nullptr);
    return;
  }

  std::vector<FileSystem> file_systems;
  for (auto file_system_path : file_system_paths) {
    base::FilePath path = base::FilePath::FromUTF8Unsafe(file_system_path);
    std::string file_system_id = RegisterFileSystem(GetDevToolsWebContents(),
                                                    path);
    FileSystem file_system = CreateFileSystemStruct(GetDevToolsWebContents(),
                                                    file_system_id,
                                                    file_system_path);
    file_systems.push_back(file_system);
  }

  base::ListValue file_system_value;
  for (const auto& file_system : file_systems)
    file_system_value.Append(CreateFileSystemValue(file_system));
  web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                    &file_system_value, nullptr, nullptr);
}

void CommonWebContentsDelegate::DevToolsAddFileSystem(
    const base::FilePath& file_system_path) {
  base::FilePath path = file_system_path;
  if (path.empty()) {
    file_dialog::Filters filters;
    base::FilePath default_path;
    std::vector<base::FilePath> paths;
    int flag = file_dialog::FILE_DIALOG_OPEN_DIRECTORY;
    if (!file_dialog::ShowOpenDialog(owner_window(), "", "", default_path,
                                     filters, flag, &paths))
      return;

    path = paths[0];
  }

  std::string file_system_id = RegisterFileSystem(GetDevToolsWebContents(),
                                                  path);
  if (IsDevToolsFileSystemAdded(GetDevToolsWebContents(), path.AsUTF8Unsafe()))
    return;

  FileSystem file_system = CreateFileSystemStruct(GetDevToolsWebContents(),
                                                 file_system_id,
                                                 path.AsUTF8Unsafe());
  std::unique_ptr<base::DictionaryValue> file_system_value(
      CreateFileSystemValue(file_system));

  auto pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->SetWithoutPathExpansion(
      path.AsUTF8Unsafe(), base::Value::CreateNullValue());

  web_contents_->CallClientFunction("DevToolsAPI.fileSystemAdded",
                                    file_system_value.get(),
                                    nullptr, nullptr);
}

void CommonWebContentsDelegate::DevToolsRemoveFileSystem(
    const base::FilePath& file_system_path) {
  if (!web_contents_)
    return;

  std::string path = file_system_path.AsUTF8Unsafe();
  storage::IsolatedContext::GetInstance()->
      RevokeFileSystemByPath(file_system_path);

  auto pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->RemoveWithoutPathExpansion(path, nullptr);

  base::StringValue file_system_path_value(path);
  web_contents_->CallClientFunction("DevToolsAPI.fileSystemRemoved",
                                    &file_system_path_value,
                                    nullptr, nullptr);
}

void CommonWebContentsDelegate::DevToolsIndexPath(
    int request_id,
    const std::string& file_system_path) {
  if (!IsDevToolsFileSystemAdded(GetDevToolsWebContents(), file_system_path)) {
    OnDevToolsIndexingDone(request_id, file_system_path);
    return;
  }
  if (devtools_indexing_jobs_.count(request_id) != 0)
    return;
  devtools_indexing_jobs_[request_id] =
      scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>(
          devtools_file_system_indexer_->IndexPath(
              file_system_path,
              base::Bind(
                  &CommonWebContentsDelegate::OnDevToolsIndexingWorkCalculated,
                  base::Unretained(this),
                  request_id,
                  file_system_path),
              base::Bind(&CommonWebContentsDelegate::OnDevToolsIndexingWorked,
                         base::Unretained(this),
                         request_id,
                         file_system_path),
              base::Bind(&CommonWebContentsDelegate::OnDevToolsIndexingDone,
                         base::Unretained(this),
                         request_id,
                         file_system_path)));
}

void CommonWebContentsDelegate::DevToolsStopIndexing(int request_id) {
  auto it = devtools_indexing_jobs_.find(request_id);
  if (it == devtools_indexing_jobs_.end())
    return;
  it->second->Stop();
  devtools_indexing_jobs_.erase(it);
}

void CommonWebContentsDelegate::DevToolsSearchInPath(
    int request_id,
    const std::string& file_system_path,
    const std::string& query) {
  if (!IsDevToolsFileSystemAdded(GetDevToolsWebContents(), file_system_path)) {
    OnDevToolsSearchCompleted(request_id,
                              file_system_path,
                              std::vector<std::string>());
    return;
  }
  devtools_file_system_indexer_->SearchInPath(
      file_system_path,
      query,
      base::Bind(&CommonWebContentsDelegate::OnDevToolsSearchCompleted,
                 base::Unretained(this),
                 request_id,
                 file_system_path));
}

void CommonWebContentsDelegate::OnDevToolsSaveToFile(
    const std::string& url) {
  // Notify DevTools.
  base::StringValue url_value(url);
  web_contents_->CallClientFunction(
      "DevToolsAPI.savedURL", &url_value, nullptr, nullptr);
}

void CommonWebContentsDelegate::OnDevToolsAppendToFile(
    const std::string& url) {
  // Notify DevTools.
  base::StringValue url_value(url);
  web_contents_->CallClientFunction(
      "DevToolsAPI.appendedToURL", &url_value, nullptr, nullptr);
}

void CommonWebContentsDelegate::OnDevToolsIndexingWorkCalculated(
    int request_id,
    const std::string& file_system_path,
    int total_work) {
  base::FundamentalValue request_id_value(request_id);
  base::StringValue file_system_path_value(file_system_path);
  base::FundamentalValue total_work_value(total_work);
  web_contents_->CallClientFunction("DevToolsAPI.indexingTotalWorkCalculated",
                                    &request_id_value,
                                    &file_system_path_value,
                                    &total_work_value);
}

void CommonWebContentsDelegate::OnDevToolsIndexingWorked(
    int request_id,
    const std::string& file_system_path,
    int worked) {
  base::FundamentalValue request_id_value(request_id);
  base::StringValue file_system_path_value(file_system_path);
  base::FundamentalValue worked_value(worked);
  web_contents_->CallClientFunction("DevToolsAPI.indexingWorked",
                                    &request_id_value,
                                    &file_system_path_value,
                                    &worked_value);
}

void CommonWebContentsDelegate::OnDevToolsIndexingDone(
    int request_id,
    const std::string& file_system_path) {
  devtools_indexing_jobs_.erase(request_id);
  base::FundamentalValue request_id_value(request_id);
  base::StringValue file_system_path_value(file_system_path);
  web_contents_->CallClientFunction("DevToolsAPI.indexingDone",
                                    &request_id_value,
                                    &file_system_path_value,
                                    nullptr);
}

void CommonWebContentsDelegate::OnDevToolsSearchCompleted(
    int request_id,
    const std::string& file_system_path,
    const std::vector<std::string>& file_paths) {
  base::ListValue file_paths_value;
  for (const auto& file_path : file_paths) {
    file_paths_value.AppendString(file_path);
  }
  base::FundamentalValue request_id_value(request_id);
  base::StringValue file_system_path_value(file_system_path);
  web_contents_->CallClientFunction("DevToolsAPI.searchCompleted",
                                    &request_id_value,
                                    &file_system_path_value,
                                    &file_paths_value);
}

void CommonWebContentsDelegate::SetHtmlApiFullscreen(bool enter_fullscreen) {
  if (owner_window_) {
    // Window is already in fullscreen mode, save the state.
    if (enter_fullscreen && owner_window_->IsFullscreen()) {
      native_fullscreen_ = true;
      html_fullscreen_ = true;
      return;
    }

    // Exit html fullscreen state but not window's fullscreen mode.
    if (!enter_fullscreen && native_fullscreen_) {
      html_fullscreen_ = false;
      return;
    }

    owner_window_->SetFullScreen(enter_fullscreen);
  }
  html_fullscreen_ = enter_fullscreen;
  native_fullscreen_ = false;
}

}  // namespace atom
