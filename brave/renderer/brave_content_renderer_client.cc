// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/renderer/brave_content_renderer_client.h"

#include "atom/renderer/content_settings_manager.h"
#include "brave/renderer/printing/brave_print_render_frame_helper_delegate.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/secure_origin_whitelist.h"
#include "chrome/renderer/chrome_render_frame_observer.h"
#include "chrome/renderer/chrome_render_thread_observer.h"
#include "chrome/renderer/chrome_render_view_observer.h"
#include "chrome/renderer/content_settings_observer.h"
#include "chrome/renderer/net/net_error_helper.h"
#include "chrome/renderer/pepper/pepper_helper.h"
#include "chrome/renderer/plugins/non_loadable_plugin_placeholder.h"
#include "chrome/renderer/plugins/plugin_uma.h"
#include "content/public/common/content_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "components/autofill/content/renderer/autofill_agent.h"
#include "components/autofill/content/renderer/password_autofill_agent.h"
#include "components/autofill/content/renderer/password_generation_agent.h"
#include "components/network_hints/renderer/prescient_networking_dispatcher.h"
#include "components/password_manager/content/renderer/credential_manager_client.h"
#include "components/plugins/renderer/plugin_placeholder.h"
#include "components/printing/renderer/print_render_frame_helper.h"
#include "components/spellcheck/spellcheck_build_features.h"
#include "components/visitedlink/renderer/visitedlink_slave.h"
#include "components/web_cache/renderer/web_cache_impl.h"
#include "extensions/features/features.h"
#include "third_party/WebKit/public/platform/URLConversion.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebSocketHandshakeThrottle.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebPlugin.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
#if defined(OS_WIN)
#include <shlobj.h>
#include "base/command_line.h"
#include "atom/common/options_switches.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/renderer/extensions/chrome_extensions_renderer_client.h"
#endif

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/renderer/spellcheck.h"
#include "components/spellcheck/renderer/spellcheck_provider.h"

#if BUILDFLAG(HAS_SPELLCHECK_PANEL)
#include "components/spellcheck/renderer/spellcheck_panel.h"
#endif  // BUILDFLAG(HAS_SPELLCHECK_PANEL)
#endif

using autofill::AutofillAgent;
using autofill::PasswordAutofillAgent;
using autofill::PasswordGenerationAgent;
using blink::WebLocalFrame;
using blink::WebPlugin;
using blink::WebPluginParams;
using blink::WebSecurityOrigin;
using blink::WebSecurityPolicy;
using blink::WebString;



namespace brave {

BraveContentRendererClient::BraveContentRendererClient() {
}

void BraveContentRendererClient::RenderThreadStarted() {
  content::RenderThread* thread = content::RenderThread::Get();

  content_settings_manager_ = atom::ContentSettingsManager::GetInstance();
  #if defined(OS_WIN)
    // Set ApplicationUserModelID in renderer process.
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    base::string16 app_id =
        command_line->GetSwitchValueNative(atom::switches::kAppUserModelId);
    if (!app_id.empty()) {
      SetCurrentProcessExplicitAppUserModelID(app_id.c_str());
    }
  #endif

  chrome_observer_.reset(new ChromeRenderThreadObserver());
  web_cache_impl_.reset(new web_cache::WebCacheImpl());

#if BUILDFLAG(ENABLE_EXTENSIONS)
  ChromeExtensionsRendererClient::GetInstance()->RenderThreadStarted();
#endif

  thread->AddObserver(chrome_observer_.get());

  prescient_networking_dispatcher_.reset(
      new network_hints::PrescientNetworkingDispatcher());

  for (auto& origin : secure_origin_whitelist::GetWhitelist()) {
    WebSecurityPolicy::AddOriginTrustworthyWhiteList(WebSecurityOrigin(origin));
  }

  for (auto& scheme :
       secure_origin_whitelist::GetSchemesBypassingSecureContextCheck()) {
    WebSecurityPolicy::AddSchemeToBypassSecureContextWhitelist(
        WebString::FromUTF8(scheme));
  }

#if BUILDFLAG(ENABLE_SPELLCHECK)
  if (!spellcheck_) {
    spellcheck_.reset(new SpellCheck(this));
    thread->AddObserver(spellcheck_.get());
  }
#endif
}

unsigned long long BraveContentRendererClient::VisitedLinkHash(  // NOLINT
    const char* canonical_url, size_t length) {
  return chrome_observer_->visited_link_slave()->ComputeURLFingerprint(
      canonical_url, length);
}

bool BraveContentRendererClient::IsLinkVisited(unsigned long long link_hash) {  // NOLINT
  return chrome_observer_->visited_link_slave()->IsVisited(link_hash);
}

blink::WebPrescientNetworking*
BraveContentRendererClient::GetPrescientNetworking() {
  return prescient_networking_dispatcher_.get();
}

void BraveContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  ChromeRenderFrameObserver* render_frame_observer =
      new ChromeRenderFrameObserver(render_frame);
  service_manager::BinderRegistry* registry = render_frame_observer->registry();

  bool should_whitelist_for_content_settings = false;
  extensions::Dispatcher* ext_dispatcher = NULL;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  ext_dispatcher =
      ChromeExtensionsRendererClient::GetInstance()->extension_dispatcher();
#endif
  ContentSettingsObserver* content_settings = new ContentSettingsObserver(
      render_frame, ext_dispatcher, should_whitelist_for_content_settings,
      registry);
  if (chrome_observer_.get()) {
    content_settings->SetContentSettingRules(
        chrome_observer_->content_setting_rules());
  }

  if (content_settings_manager_) {
    content_settings->SetContentSettingsManager(content_settings_manager_);
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  ChromeExtensionsRendererClient::GetInstance()->RenderFrameCreated(
      render_frame, registry);
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
  new PepperHelper(render_frame);
#endif

  new NetErrorHelper(render_frame);

  PasswordAutofillAgent* password_autofill_agent =
      new PasswordAutofillAgent(render_frame, registry);
  PasswordGenerationAgent* password_generation_agent =
      new PasswordGenerationAgent(render_frame, password_autofill_agent,
                                  registry);
  new AutofillAgent(render_frame, password_autofill_agent,
                    password_generation_agent, registry);
#if BUILDFLAG(ENABLE_PRINTING)
  new printing::PrintRenderFrameHelper(
      render_frame, base::MakeUnique<BravePrintRenderFrameHelperDelegate>());
#endif

#if BUILDFLAG(ENABLE_SPELLCHECK)
  new SpellCheckProvider(render_frame, spellcheck_.get(), this);
#if BUILDFLAG(HAS_SPELLCHECK_PANEL)
  new SpellCheckPanel(render_frame, registry, this);
#endif  // BUILDFLAG(HAS_SPELLCHECK_PANEL)
#endif
}

void BraveContentRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  new ChromeRenderViewObserver(render_view, web_cache_impl_.get());

  new password_manager::CredentialManagerClient(render_view);

#if BUILDFLAG(ENABLE_SPELLCHECK)
  // This is a workaround keeping the behavior that, the Blink side spellcheck
  // enabled state is initialized on RenderView creation.
  // TODO(xiaochengh): Design better way to sync between Chrome-side and
  // Blink-side spellcheck enabled states.  See crbug.com/710097.
  if (SpellCheckProvider* provider =
          SpellCheckProvider::Get(render_view->GetMainRenderFrame()))
    provider->EnableSpellcheck(spellcheck_->IsSpellcheckEnabled());
#endif
}

bool BraveContentRendererClient::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    const WebPluginParams& params,
    WebPlugin** plugin) {
  std::string orig_mime_type = params.mime_type.Utf8();
  if (orig_mime_type == content::kBrowserPluginMimeType)
    return false;

  GURL url(params.url);
#if BUILDFLAG(ENABLE_PLUGINS)
  chrome::mojom::PluginInfoPtr plugin_info = chrome::mojom::PluginInfo::New();
  GetPluginInfoHost()->GetPluginInfo(
      render_frame->GetRoutingID(), url,
      render_frame->GetWebFrame()->Top()->GetSecurityOrigin(), orig_mime_type,
      &plugin_info);
  *plugin = CreatePlugin(render_frame, params, *plugin_info);
#else  // !BUILDFLAG(ENABLE_PLUGINS)
  PluginUMAReporter::GetInstance()->ReportPluginMissing(orig_mime_type, url);
  *plugin = NonLoadablePluginPlaceholder::CreateNotSupportedPlugin(
                render_frame, render_frame->GetWebFrame(), params)
                ->plugin();
#endif  // BUILDFLAG(ENABLE_PLUGINS)
  return true;
}

bool BraveContentRendererClient::WillSendRequest(
    blink::WebLocalFrame* frame,
    ui::PageTransition transition_type,
    const blink::WebURL& url,
    std::vector<std::unique_ptr<content::URLLoaderThrottle>>* throttles,
    GURL* new_url) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (ChromeExtensionsRendererClient::GetInstance()->WillSendRequest(
          frame, transition_type, url, new_url)) {
    return true;
  }
#endif

  return false;
}

std::unique_ptr<blink::WebSocketHandshakeThrottle>
BraveContentRendererClient::CreateWebSocketHandshakeThrottle() {
  return nullptr;
}

}  // namespace brave
