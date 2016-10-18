// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_EXTENSIONS_API_GUEST_VIEW_TAB_VIEW_TAB_VIEW_INTERNAL_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_GUEST_VIEW_TAB_VIEW_TAB_VIEW_INTERNAL_API_H_

#include <stdint.h>

#include "base/macros.h"
#include "extensions/browser/api/execute_code_function.h"
#include "extensions/browser/extension_function.h"
#include "brave/browser/guest_view/tab_view/tab_view_guest.h"

class TabViewInternalExtensionFunction : public UIThreadExtensionFunction {
 public:
  TabViewInternalExtensionFunction() {}

 protected:
  ~TabViewInternalExtensionFunction() override {}
  bool PreRunValidation(std::string* error) override;

  brave::TabViewGuest* guest_ = nullptr;
};

class TabViewInternalGetTabIDFunction
    : public TabViewInternalExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("tabViewInternal.getTabID",
                             UNKNOWN);

  TabViewInternalGetTabIDFunction();

 protected:
  ~TabViewInternalGetTabIDFunction() override;
  ResponseAction Run() override;

  DISALLOW_COPY_AND_ASSIGN(TabViewInternalGetTabIDFunction);
};

#endif  // BRAVE_BROWSER_EXTENSIONS_API_GUEST_VIEW_TAB_VIEW_TAB_VIEW_INTERNAL_API_H_
