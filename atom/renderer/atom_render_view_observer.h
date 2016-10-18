// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_
#define ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_

#include "base/strings/string16.h"
#include "content/public/renderer/render_view_observer.h"

namespace base {
class ListValue;
}

namespace web_cache {
class WebCacheImpl;
}

namespace atom {

class AtomRendererClient;

class AtomRenderViewObserver : public content::RenderViewObserver {
 public:
  explicit AtomRenderViewObserver(content::RenderView* render_view,
                                  AtomRendererClient* renderer_client,
                                  web_cache::WebCacheImpl* web_cache_impl);

 protected:
  virtual ~AtomRenderViewObserver();

 private:
  // content::RenderViewObserver implementation.
  void DidCreateDocumentElement(blink::WebLocalFrame* frame) override;
  void DraggableRegionsChanged(blink::WebFrame* frame) override;
  void OnDestruct() override;
  void Navigate(const GURL& url) override;

  // Whether the document object has been created.
  web_cache::WebCacheImpl* web_cache_impl_;  // not owned
  bool document_created_;

  DISALLOW_COPY_AND_ASSIGN(AtomRenderViewObserver);
};

}  // namespace atom

#endif  // ATOM_RENDERER_ATOM_RENDER_VIEW_OBSERVER_H_
