// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// vector_icons.h.template is used to generate vector_icons.h. Edit the former
// rather than the latter.

#ifndef CHROME_APP_VECTOR_ICONS_VECTOR_ICONS_H_
#define CHROME_APP_VECTOR_ICONS_VECTOR_ICONS_H_

namespace gfx {
struct VectorIcon;
}

#define VECTOR_ICON_TEMPLATE_H(icon_name) \
extern const gfx::VectorIcon icon_name;

VECTOR_ICON_TEMPLATE_H(kBrowserToolsUpdateIcon)
VECTOR_ICON_TEMPLATE_H(kSecurityIcon)
VECTOR_ICON_TEMPLATE_H(kUsbSecurityKeyIcon)

#undef VECTOR_ICON_TEMPLATE_H

#endif  // CHROME_APP_VECTOR_ICONS_VECTOR_ICONS_H_
