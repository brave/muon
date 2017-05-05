// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/search.h"

#include <stddef.h>

#include "content/public/browser/web_contents.h"

namespace search {

bool IsInstantNTP(const content::WebContents* contents) {
  return false;
}

}  // namespace search
