// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prerender/prerender_manager.h"

#include <stddef.h>

#include "chrome/browser/prerender/prerender_contents.h"
#include "content/public/browser/web_contents.h"

using content::WebContents;

namespace prerender {

PrerenderContents* PrerenderManager::GetPrerenderContents(
    const content::WebContents* web_contents) const {
  return nullptr;
}

}  // namespace prerender
