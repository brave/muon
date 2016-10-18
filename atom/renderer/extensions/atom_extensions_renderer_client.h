// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDERER_CLIENT_H_
#define ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDERER_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "extensions/renderer/extensions_renderer_client.h"
#include "ui/base/page_transition_types.h"

class GURL;

namespace blink {
struct WebPluginParams;
class WebFrame;
class WebLocalFrame;
}

namespace content {
class RenderFrame;
class RenderView;
}

namespace extensions {

class Dispatcher;
class ExtensionsGuestViewContainerDispatcher;
class RendererPermissionsPolicyDelegate;
class AtomExtensionsDispatcherDelegate;

class AtomExtensionsRendererClient : public ExtensionsRendererClient {
 public:
  AtomExtensionsRendererClient();
  static AtomExtensionsRendererClient* GetInstance();

  // extensions::ExtensionsRendererClient implementation.
  bool IsIncognitoProcess() const override;
  int GetLowestIsolatedWorldId() const override;

  void RenderThreadStarted();
  void RenderFrameCreated(content::RenderFrame* render_frame);
  void RenderViewCreated(content::RenderView* render_view);
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            const blink::WebPluginParams& params);
  bool AllowPopup();
  bool WillSendRequest(blink::WebFrame* frame,
                        ui::PageTransition transition_type,
                        const GURL& url,
                        GURL* new_url);

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame);
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame);

  static bool ShouldFork(blink::WebLocalFrame* frame,
                         const GURL& url,
                         bool is_initial_navigation,
                         bool is_server_redirect,
                         bool* send_referrer);
  extensions::Dispatcher* extension_dispatcher() {
    return extension_dispatcher_.get();
  }

 private:
  std::unique_ptr<AtomExtensionsDispatcherDelegate>
      extension_dispatcher_delegate_;
  std::unique_ptr<Dispatcher> extension_dispatcher_;
  std::unique_ptr<RendererPermissionsPolicyDelegate>
      permissions_policy_delegate_;
  std::unique_ptr<extensions::ExtensionsGuestViewContainerDispatcher>
      guest_view_container_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsRendererClient);
};

}  // namespace extensions

#endif  // ATOM_RENDERER_EXTENSIONS_ATOM_EXTENSIONS_RENDERER_CLIENT_H_
