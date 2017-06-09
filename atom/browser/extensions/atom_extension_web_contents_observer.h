// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_WEB_CONTENTS_OBSERVER_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_WEB_CONTENTS_OBSERVER_H_

#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/extension_web_contents_observer.h"

namespace content {
class RenderViewHost;
}

namespace extensions {

class AtomExtensionWebContentsObserver
    : public ExtensionWebContentsObserver,
      public content::WebContentsUserData<AtomExtensionWebContentsObserver> {
 public:
  ~AtomExtensionWebContentsObserver() override;

 private:
  friend class content::WebContentsUserData<AtomExtensionWebContentsObserver>;

  explicit AtomExtensionWebContentsObserver(
      content::WebContents* web_contents);

  void RenderViewCreated(content::RenderViewHost* render_view_host);

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionWebContentsObserver);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSION_WEB_CONTENTS_OBSERVER_H_
