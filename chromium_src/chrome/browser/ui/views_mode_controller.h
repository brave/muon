// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MODE_CONTROLLER__MAC_VIEWS_BROWSER_H_
#define CHROME_BROWSER_UI_VIEWS_MODE_CONTROLLER__MAC_VIEWS_BROWSER_H_

#include "../../../chrome/browser/ui/views_mode_controller.h"

#include "ui/base/ui_base_features.h"

#if defined(OS_MACOSX) && !BUILDFLAG(MAC_VIEWS_BROWSER)
namespace features {
UI_BASE_EXPORT extern const base::Feature kViewsBrowserWindows;
UI_BASE_EXPORT bool IsViewsBrowserCocoa();
}  // namespace features

namespace views_mode_controller {
bool IsViewsBrowserCocoa();
}  // namespace views_mode_controller
#endif  //  !BUILDFLAG(MAC_VIEWS_BROWSER)

#endif  // CHROME_BROWSER_UI_VIEWS_MODE_CONTROLLER__MAC_VIEWS_BROWSER_H_
