// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_WINDOW_H_
#define CHROME_BROWSER_UI_BROWSER_WINDOW_H_

#include "ui/base/base_window.h"

////////////////////////////////////////////////////////////////////////////////
// BrowserWindow interface
//  An interface implemented by the "view" of the Browser window.
//  This interface includes ui::BaseWindow methods as well as Browser window
//  specific methods.
//
// NOTE: All getters may return NULL.
//
class BrowserWindow : public ui::BaseWindow {
 public:
  virtual ~BrowserWindow() {}
};

#endif  // CHROME_BROWSER_UI_BROWSER_WINDOW_H_
