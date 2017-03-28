// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_WINDOW_H_
#define CHROME_BROWSER_UI_BROWSER_WINDOW_H_

#include "ui/base/base_window.h"

class BrowserWindow : public ui::BaseWindow {
 public:
  virtual ~BrowserWindow() {}
  virtual bool IsVisibleOnAllWorkspaces() { return false; }
  virtual std::string GetWorkspace() { return std::string(); }
};

#endif  // CHROME_BROWSER_UI_BROWSER_WINDOW_H_
