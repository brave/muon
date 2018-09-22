// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_HOST_DELEGATE_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_HOST_DELEGATE_H_

#include <memory>
#include <string>

#include "extensions/browser/extension_host_delegate.h"

namespace extensions {

class AtomExtensionHostDelegate : public ExtensionHostDelegate {
 public:
  AtomExtensionHostDelegate();
  ~AtomExtensionHostDelegate() override;

  // ExtensionHostDelegate implementation.
  void OnExtensionHostCreated(content::WebContents* web_contents) override;
  void OnRenderViewCreatedForBackgroundPage(ExtensionHost* host) override;
  content::JavaScriptDialogManager* GetJavaScriptDialogManager() override;
  void CreateTab(std::unique_ptr<content::WebContents> web_contents,
                 const std::string& extension_id,
                 WindowOpenDisposition disposition,
                 const gfx::Rect& initial_rect,
                 bool user_gesture) override;
  void ProcessMediaAccessRequest(content::WebContents* web_contents,
                                 const content::MediaStreamRequest& request,
                                 content::MediaResponseCallback callback,
                                 const Extension* extension) override;
  bool CheckMediaAccessPermission(content::RenderFrameHost* render_frame_host,
                                  const GURL& security_origin,
                                  content::MediaStreamType type,
                                  const Extension* extension) override;
  ExtensionHostQueue* GetExtensionHostQueue() const override;
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_HOST_DELEGATE_H_
