// Copyright 2015 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_
#define BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "chrome/renderer/chrome_content_renderer_client.h"

#include "base/compiler_specific.h"

namespace atom {
class ContentSettingsManager;
}

namespace blink {
class WebPrescientNetworking;
class WebLocalFrame;
class WebPlugin;
}

namespace content {
class RenderFrame;
}

class SpellCheck;

namespace brave {

class BraveContentRendererClient : public ChromeContentRendererClient {
 public:
  BraveContentRendererClient();

  // content::ContentRendererClient:
  void RenderThreadStarted() override;
  void RenderFrameCreated(content::RenderFrame*) override;
  void RenderViewCreated(content::RenderView*) override;
  bool OverrideCreatePlugin(
      content::RenderFrame* render_frame,
      const blink::WebPluginParams& params,
      blink::WebPlugin** plugin) override;
  unsigned long long VisitedLinkHash(  // NOLINT
      const char* canonical_url, size_t length) override;
  bool IsLinkVisited(unsigned long long link_hash) override;  // NOLINT

  blink::WebPrescientNetworking* GetPrescientNetworking() override;

  bool WillSendRequest(
    blink::WebLocalFrame* frame,
    ui::PageTransition transition_type,
    const blink::WebURL& url,
    GURL* new_url) override;
  std::unique_ptr<blink::WebSocketHandshakeThrottle>
      CreateWebSocketHandshakeThrottle() override;

 private:
  atom::ContentSettingsManager* content_settings_manager_;  // not owned

  std::unique_ptr<SpellCheck> spellcheck_;

  std::unique_ptr<ChromeRenderThreadObserver> chrome_observer_;
  std::unique_ptr<web_cache::WebCacheImpl> web_cache_impl_;

  std::unique_ptr<network_hints::PrescientNetworkingDispatcher>
      prescient_networking_dispatcher_;

  DISALLOW_COPY_AND_ASSIGN(BraveContentRendererClient);
};

}  // namespace brave

#endif  // BRAVE_RENDERER_BRAVE_CONTENT_RENDERER_CLIENT_H_
