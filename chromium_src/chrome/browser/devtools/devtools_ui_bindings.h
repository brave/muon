// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_
#define CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_

#include "base/macros.h"

namespace content {
class WebContents;
}

// Base implementation of DevTools bindings around front-end.
class DevToolsUIBindings {
 public:

  explicit DevToolsUIBindings(content::WebContents* web_contents);
  ~DevToolsUIBindings();

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsUIBindings);
};

#endif  // CHROME_BROWSER_DEVTOOLS_DEVTOOLS_UI_BINDINGS_H_
