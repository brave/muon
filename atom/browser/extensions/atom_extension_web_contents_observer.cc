// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "atom/browser/extensions/atom_extension_web_contents_observer.h"

#include "chrome/common/url_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/url_constants.h"
#include "extensions/browser/extension_error.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/extension_urls.h"
#include "extensions/renderer/console.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    extensions::AtomExtensionWebContentsObserver);

namespace extensions {

AtomExtensionWebContentsObserver::AtomExtensionWebContentsObserver(
    content::WebContents* web_contents)
    : ExtensionWebContentsObserver(web_contents) {}

AtomExtensionWebContentsObserver::~AtomExtensionWebContentsObserver() {}

void AtomExtensionWebContentsObserver::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  ExtensionWebContentsObserver::RenderFrameCreated(render_frame_host);

  const Extension* extension = GetExtensionFromFrame(render_frame_host, false);
  if (!extension)
    return;

  int process_id = render_frame_host->GetProcess()->GetID();
  auto* policy = content::ChildProcessSecurityPolicy::GetInstance();

  // Components of chrome that are implemented as extensions or platform apps
  // are allowed to use chrome://resources/ and chrome://theme/ URLs.
  if ((extension->is_extension() || extension->is_platform_app()) &&
      Manifest::IsComponentLocation(extension->location())) {
    policy->GrantOrigin(
        process_id, url::Origin::Create(GURL(content::kChromeUIResourcesURL)));
    policy->GrantOrigin(process_id,
                        url::Origin::Create(GURL(chrome::kChromeUIThemeURL)));
  }

  // Extensions, legacy packaged apps, and component platform apps are allowed
  // to use chrome://favicon/ and chrome://extension-icon/ URLs. Hosted apps are
  // not allowed because they are served via web servers (and are generally
  // never given access to Chrome APIs).
  if (extension->is_extension() ||
      extension->is_legacy_packaged_app() ||
      (extension->is_platform_app() &&
       Manifest::IsComponentLocation(extension->location()))) {
    policy->GrantOrigin(process_id,
                        url::Origin::Create(GURL(chrome::kChromeUIFaviconURL)));
    policy->GrantOrigin(
        process_id,
        url::Origin::Create(GURL(chrome::kChromeUIExtensionIconURL)));
  }
}

void AtomExtensionWebContentsObserver::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  ExtensionWebContentsObserver::RenderFrameDeleted(render_frame_host);
}

}  // namespace extensions
