// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_renderer_client.h"

#include <string>
#include <vector>

#include "atom/common/api/api_messages.h"
#include "atom/common/color_util.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/atom_render_view_observer.h"
#include "base/command_line.h"
#include "chrome/renderer/media/chrome_key_systems.h"
#include "chrome/renderer/pepper/pepper_helper.h"
#include "chrome/renderer/tts_dispatcher.h"
#include "components/web_cache/renderer/web_cache_impl.h"
#include "content/public/common/content_constants.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "extensions/renderer/guest_view/extensions_guest_view_container.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/web/WebCustomElement.h"
#include "third_party/WebKit/public/web/WebPluginParams.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#endif

#if defined(OS_WIN)
#include <shlobj.h>
#endif

namespace atom {

AtomRendererClient::AtomRendererClient() {
  // Parse --standard-schemes=scheme1,scheme2
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  std::string custom_schemes = command_line->GetSwitchValueASCII(
      switches::kStandardSchemes);
  if (!custom_schemes.empty()) {
    std::vector<std::string> schemes_list = base::SplitString(
        custom_schemes, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    for (const std::string& scheme : schemes_list)
      url::AddStandardScheme(scheme.c_str(), url::SCHEME_WITHOUT_PORT);
  }
}

AtomRendererClient::~AtomRendererClient() {
}

void AtomRendererClient::RenderThreadStarted() {
  web_cache_impl_.reset(new web_cache::WebCacheImpl());

#if defined(OS_WIN)
  // Set ApplicationUserModelID in renderer process.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  base::string16 app_id =
      command_line->GetSwitchValueNative(switches::kAppUserModelId);
  if (!app_id.empty()) {
    SetCurrentProcessExplicitAppUserModelID(app_id.c_str());
  }
#endif
}

void AtomRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new PepperHelper(render_frame);
  // Allow file scheme to handle service worker by default.
  // FIXME(zcbenz): Can this be moved elsewhere?
  blink::WebSecurityPolicy::registerURLSchemeAsAllowingServiceWorkers("file");
}

void AtomRendererClient::RenderViewCreated(content::RenderView* render_view) {
  new AtomRenderViewObserver(render_view, this, web_cache_impl_.get());
}

blink::WebSpeechSynthesizer* AtomRendererClient::OverrideSpeechSynthesizer(
    blink::WebSpeechSynthesizerClient* client) {
  return new TtsDispatcher(client);
}

bool AtomRendererClient::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
  // TODO(bridiver) - content settings handles the actual permission, but
  // we should check it here so we can return true
  return false;
}

content::BrowserPluginDelegate* AtomRendererClient::CreateBrowserPluginDelegate(
    content::RenderFrame* render_frame,
    const std::string& mime_type,
    const GURL& original_url) {
  if (mime_type == content::kBrowserPluginMimeType) {
    return new extensions::ExtensionsGuestViewContainer(render_frame);
  } else {
    return nullptr;
  }
}

void AtomRendererClient::AddSupportedKeySystems(
    std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems) {
  AddChromeKeySystems(key_systems);
}

}  // namespace atom
