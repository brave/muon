// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// vector_icons.cc.template is used to generate vector_icons.cc. Edit the former
// rather than the latter.

#include "chrome/app/vector_icons/vector_icons.h"

#include "ui/gfx/vector_icon_types.h"

// Using upstream VECTOR_ICON_TEMPLATE causes compile failure on Windows.
#define VECTOR_ICON_TEMPLATE(icon_name)                             \
  const gfx::VectorIcon icon_name;

VECTOR_ICON_TEMPLATE(kBrowserToolsUpdateIcon)
VECTOR_ICON_TEMPLATE(kUsbSecurityKeyIcon)
