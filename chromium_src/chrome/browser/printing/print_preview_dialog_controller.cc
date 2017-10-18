// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_preview_dialog_controller.h"

#include <stddef.h>

#include <algorithm>
#include <string>
#include <vector>

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/web_contents.h"

using content::WebContents;

namespace printing {

PrintPreviewDialogController::PrintPreviewDialogController()
    : waiting_for_new_preview_page_(false),
      is_creating_print_preview_dialog_(false) {}

// static
PrintPreviewDialogController* PrintPreviewDialogController::GetInstance() {
  return nullptr;
}

// static
void PrintPreviewDialogController::PrintPreview(WebContents* initiator) {
  if (initiator->ShowingInterstitialPage() || initiator->IsCrashed())
    return;

  PrintViewManager* print_view_manager =
      PrintViewManager::FromWebContents(initiator);
  if (print_view_manager)
    print_view_manager->PrintPreviewDone();
}

WebContents* PrintPreviewDialogController::GetPrintPreviewForContents(
    WebContents* contents) const {
  return nullptr;
}

// static
bool PrintPreviewDialogController::IsPrintPreviewURL(const GURL& url) {
  return (url.SchemeIs(content::kChromeUIScheme) &&
          url.host_piece() == chrome::kChromeUIPrintHost);
}

PrintPreviewDialogController::~PrintPreviewDialogController() {}

WebContents* PrintPreviewDialogController::GetInitiator(
    WebContents* preview_dialog) {
  return nullptr;
}

void PrintPreviewDialogController::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {}

}  // namespace printing
