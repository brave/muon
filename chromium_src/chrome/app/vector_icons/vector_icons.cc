// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// vector_icons.cc.template is used to generate vector_icons.cc. Edit the former
// rather than the latter.

#include "chrome/app/vector_icons/vector_icons.h"

#include "base/logging.h"
#include "ui/gfx/vector_icon_types.h"

#define PATH_ELEMENT_TEMPLATE(path_name, ...) \
static constexpr gfx::PathElement path_name[] = {__VA_ARGS__};

#define VECTOR_ICON_TEMPLATE(icon_name, path_name, path_name_1x) \
const gfx::VectorIcon icon_name = { path_name , path_name_1x };

using namespace gfx;

VECTOR_ICON_TEMPLATE(kBrowserToolsUpdateIcon, nullptr, nullptr)
VECTOR_ICON_TEMPLATE(kUsbSecurityKeyIcon, nullptr, nullptr)
