// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/chrome_select_file_policy.h"

ChromeSelectFilePolicy::ChromeSelectFilePolicy(
    content::WebContents* source_contents)
    : source_contents_(source_contents) {}

ChromeSelectFilePolicy::~ChromeSelectFilePolicy() {}

bool ChromeSelectFilePolicy::CanOpenSelectFileDialog() {
  return FileSelectDialogsAllowed();
}

void ChromeSelectFilePolicy::SelectFileDenied() {}

// static
bool ChromeSelectFilePolicy::FileSelectDialogsAllowed() {
  return true;
}
