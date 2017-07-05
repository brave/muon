// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "atom/browser/atom_browser_client.h"

#include "base/memory/ptr_util.h"

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/api/atom_api_protocol.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_quota_permission_context.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "atom/browser/atom_speech_recognition_manager_delegate.h"
#include "atom/browser/native_window.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/window_list.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_host/pepper/chrome_browser_pepper_host_factory.h"
#include "chrome/browser/speech/tts_message_filter.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/web_preferences.h"
#include "extensions/features/features.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "ppapi/host/ppapi_host.h"
#include "ui/base/l10n/l10n_util.h"
#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"

#if defined(OS_WIN)
#include <Objbase.h>
#include <shlobj.h>
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/constants.h"
#include "extensions/common/switches.h"
#endif

namespace atom {

namespace {

// Custom schemes to be registered to handle service worker.
std::string g_custom_service_worker_schemes = "";  // NOLINT

}  // namespace

void AtomBrowserClient::SetCustomServiceWorkerSchemes(
    const std::vector<std::string>& schemes) {
  g_custom_service_worker_schemes = base::JoinString(schemes, ",");
}

AtomBrowserClient::AtomBrowserClient() : delegate_(nullptr) {
}

AtomBrowserClient::~AtomBrowserClient() {
}

content::WebContents* AtomBrowserClient::GetWebContentsFromProcessID(
    int process_id) {
  // If the process is a pending process, we should use the old one.
  if (base::ContainsKey(pending_processes_, process_id))
    process_id = pending_processes_[process_id];

  // Certain render process will be created with no associated render view,
  // for example: ServiceWorker.
  return WebContentsPreferences::GetWebContentsFromProcessID(process_id);
}

content::SpeechRecognitionManagerDelegate*
    AtomBrowserClient::CreateSpeechRecognitionManagerDelegate() {
  return new AtomSpeechRecognitionManagerDelegate;
}

void AtomBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* host, content::WebPreferences* prefs) {
  prefs->javascript_enabled = true;
  prefs->web_security_enabled = true;
  prefs->plugins_enabled = true;
  prefs->dom_paste_enabled = true;
  prefs->allow_scripts_to_close_windows = true;
  prefs->javascript_can_access_clipboard = false;
  prefs->local_storage_enabled = true;
  prefs->databases_enabled = true;
  prefs->application_cache_enabled = true;
  prefs->allow_universal_access_from_file_urls = false;
  prefs->allow_file_access_from_file_urls = false;
  prefs->hyperlink_auditing_enabled = false;
  prefs->experimental_webgl_enabled = true;
  prefs->allow_running_insecure_content = false;

  // Custom preferences of guest page.
  auto web_contents = content::WebContents::FromRenderViewHost(host);
  WebContentsPreferences::OverrideWebkitPrefs(web_contents, prefs);
}

std::string AtomBrowserClient::GetApplicationLocale() {
  return l10n_util::GetApplicationLocale("");
}

void AtomBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int process_id) {
  const base::CommandLine& browser_command_line =
      *base::CommandLine::ForCurrentProcess();

  std::string process_type = command_line->GetSwitchValueASCII("type");

  // Copy following switches to child process.
  static const char* const kCommonSwitchNames[] = {
    switches::kStandardSchemes,
    ::switches::kUserAgent,
    ::switches::kUserDataDir,  // Make logs go to the right file.
  };
  command_line->CopySwitchesFrom(
      *base::CommandLine::ForCurrentProcess(),
      kCommonSwitchNames, arraysize(kCommonSwitchNames));

  if (process_type == ::switches::kRendererProcess) {
    static const char* const kSwitchNames[] = {
#if BUILDFLAG(ENABLE_EXTENSIONS)
      extensions::switches::kAllowHTTPBackgroundPage,
      extensions::switches::kAllowLegacyExtensionManifests,
      extensions::switches::kEnableEmbeddedExtensionOptions,
      extensions::switches::kEnableExperimentalExtensionApis,
      extensions::switches::kExtensionsOnChromeURLs,
      extensions::switches::kNativeCrxBindings,
      extensions::switches::kWhitelistedExtensionID,
#endif
      ::switches::kAllowInsecureLocalhost,
      ::switches::kPpapiFlashArgs,
      ::switches::kPpapiFlashPath,
      ::switches::kPpapiFlashVersion,
    };

    command_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                                   arraysize(kSwitchNames));

  // The registered service worker schemes.
  if (!g_custom_service_worker_schemes.empty())
    command_line->AppendSwitchASCII(switches::kRegisterServiceWorkerSchemes,
                                    g_custom_service_worker_schemes);

#if defined(OS_WIN)
  // Append --app-user-model-id.
  PWSTR current_app_id;
  if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&current_app_id))) {
    command_line->AppendSwitchNative(switches::kAppUserModelId, current_app_id);
    CoTaskMemFree(current_app_id);
  }
#endif

  content::WebContents* web_contents = GetWebContentsFromProcessID(process_id);
  if (!web_contents)
    return;

  WebContentsPreferences::AppendExtraCommandLineSwitches(
      web_contents, command_line);
  } else if (process_type == ::switches::kUtilityProcess) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
    static const char* const kSwitchNames[] = {
      extensions::switches::kAllowHTTPBackgroundPage,
      extensions::switches::kEnableExperimentalExtensionApis,
      extensions::switches::kExtensionsOnChromeURLs,
      extensions::switches::kWhitelistedExtensionID,
    };

    command_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                                   arraysize(kSwitchNames));
#endif
  } else if (process_type == ::switches::kZygoteProcess) {
    static const char* const kSwitchNames[] = {
      // Load (in-process) Pepper plugins in-process in the zygote pre-sandbox.
      ::switches::kDisableBundledPpapiFlash,
      ::switches::kPpapiFlashPath,
      ::switches::kPpapiFlashVersion,
    };

    command_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                                   arraysize(kSwitchNames));
  }
}

void AtomBrowserClient::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* host) {
  host->GetPpapiHost()->AddHostFactoryFilter(
      base::WrapUnique(new ChromeBrowserPepperHostFactory(host)));
}

content::QuotaPermissionContext*
    AtomBrowserClient::CreateQuotaPermissionContext() {
  return new AtomQuotaPermissionContext;
}

void AtomBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    content::ResourceType resource_type,
    bool overridable,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(
        content::CertificateRequestResultType)>& callback) {
  if (delegate_) {
    delegate_->AllowCertificateError(
        web_contents, cert_error, ssl_info, request_url,
        resource_type, overridable, strict_enforcement,
        expired_previous_decision, callback);
  }
}

void AtomBrowserClient::SelectClientCertificate(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    net::ClientCertIdentityList client_certs,
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
  if (!client_certs.empty() && delegate_) {
    delegate_->SelectClientCertificate(web_contents, cert_request_info,
                                       std::move(client_certs),
                                       std::move(delegate));
  }
}

void AtomBrowserClient::ResourceDispatcherHostCreated() {
  g_browser_process->ResourceDispatcherHostCreated();
}

void AtomBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
    std::vector<std::string>* additional_schemes) {
  auto schemes_list = api::GetStandardSchemes();
  if (!schemes_list.empty())
    additional_schemes->insert(additional_schemes->end(),
                               schemes_list.begin(),
                               schemes_list.end());
}

brightray::BrowserMainParts* AtomBrowserClient::OverrideCreateBrowserMainParts(
    const content::MainFunctionParams&) {
  return new AtomBrowserMainParts;
}

void AtomBrowserClient::WebNotificationAllowed(
    int render_process_id,
    const base::Callback<void(bool, bool)>& callback) {
  content::WebContents* web_contents =
      WebContentsPreferences::GetWebContentsFromProcessID(render_process_id);
  if (!web_contents) {
    callback.Run(false, false);
    return;
  }
  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper) {
    callback.Run(false, false);
    return;
  }
  permission_helper->RequestWebNotificationPermission(
      base::Bind(callback, web_contents->IsAudioMuted()));
}

void AtomBrowserClient::RenderProcessHostDestroyed(
    content::RenderProcessHost* host) {
  int process_id = host->GetID();
  for (const auto& entry : pending_processes_) {
    if (entry.first == process_id || entry.second == process_id) {
      pending_processes_.erase(entry.first);
      break;
    }
  }
}

}  // namespace atom
