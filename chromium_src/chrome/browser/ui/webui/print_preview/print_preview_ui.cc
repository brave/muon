// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/print_preview/print_preview_ui.h"

// static
void PrintPreviewUI::GetCurrentPrintPreviewStatus(const PrintHostMsg_PreviewIds& ids,
                                                  bool* cancel) {
  // TODO(bridiver) - need a real implementation of this
  return;
}

// static
void PrintPreviewUI::SetInitialParams(
    content::WebContents* print_preview_dialog,
    const PrintHostMsg_RequestPrintPreview_Params& params) {}
