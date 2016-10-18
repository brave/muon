// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/renderer/atom_render_view_observer.h"

#include <string>
#include <vector>

// Put this before event_emitter_caller.h to have string16 support.
#include "atom/common/native_mate_converters/string16_converter.h"

#include "atom/browser/web_contents_preferences.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "atom/renderer/atom_renderer_client.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "components/web_cache/renderer/web_cache_impl.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message_macros.h"
#include "native_mate/dictionary.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebDraggableRegion.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "ui/base/resource/resource_bundle.h"

namespace atom {

namespace {

base::StringPiece NetResourceProvider(int key) {
  if (key == IDR_DIR_HEADER_HTML) {
    base::StringPiece html_data =
        ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
            IDR_DIR_HEADER_HTML);
    return html_data;
  }
  return base::StringPiece();
}

}  // namespace

AtomRenderViewObserver::AtomRenderViewObserver(
    content::RenderView* render_view,
    AtomRendererClient* renderer_client,
    web_cache::WebCacheImpl* web_cache_impl)
    : content::RenderViewObserver(render_view),
      web_cache_impl_(web_cache_impl),
      document_created_(false) {
  // Initialise resource for directory listing.
  net::NetModule::SetResourceProvider(NetResourceProvider);
}

AtomRenderViewObserver::~AtomRenderViewObserver() {
}

void AtomRenderViewObserver::DidCreateDocumentElement(
    blink::WebLocalFrame* frame) {
  document_created_ = true;
}

void AtomRenderViewObserver::Navigate(const GURL& url) {
  // Execute cache clear operations that were postponed until a navigation
  // event (including tab reload).
  if (web_cache_impl_)
    web_cache_impl_->ExecutePendingClearCache();
}

void AtomRenderViewObserver::DraggableRegionsChanged(blink::WebFrame* frame) {
  blink::WebVector<blink::WebDraggableRegion> webregions =
      frame->document().draggableRegions();
  std::vector<DraggableRegion> regions;
  for (auto& webregion : webregions) {
    DraggableRegion region;
    render_view()->ConvertViewportToWindowViaWidget(&webregion.bounds);
    region.bounds = webregion.bounds;
    region.draggable = webregion.draggable;
    regions.push_back(region);
  }
  Send(new AtomViewHostMsg_UpdateDraggableRegions(routing_id(), regions));
}

void AtomRenderViewObserver::OnDestruct() {
  delete this;
}


}  // namespace atom
