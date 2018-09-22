// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_UI_BASE_FEATURES_MAC_VIEWS_BROWSER_H_
#define UI_BASE_UI_BASE_FEATURES_MAC_VIEWS_BROWSER_H_

#include "../../../../ui/base/ui_base_features.h"

#if defined(OS_MACOSX) && !BUILDFLAG(MAC_VIEWS_BROWSER)
namespace features {
UI_BASE_EXPORT extern const base::Feature kViewsBrowserWindows;
UI_BASE_EXPORT bool IsViewsBrowserCocoa();
}  // namespace features

namespace views_mode_controller {
bool IsViewsBrowserCocoa();
}  // namespace views_mode_controller
#endif  //  !BUILDFLAG(MAC_VIEWS_BROWSER)

#endif  // UI_BASE_UI_BASE_FEATURES_MAC_VIEWS_BROWSER_H_
