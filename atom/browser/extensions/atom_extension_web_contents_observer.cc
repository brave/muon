// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extension_web_contents_observer.h"

#include <string>
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
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
    : ExtensionWebContentsObserver(web_contents) { }

AtomExtensionWebContentsObserver::~AtomExtensionWebContentsObserver() {}

}  // namespace extensions
