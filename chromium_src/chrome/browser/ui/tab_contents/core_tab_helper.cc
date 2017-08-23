// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tab_contents/core_tab_helper.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(CoreTabHelper);

void CoreTabHelper::OnCloseStarted() {
  NOTREACHED();
}
