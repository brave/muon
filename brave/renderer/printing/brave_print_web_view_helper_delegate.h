// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_RENDERER_PRINTING_BRAVE_PRINT_WEB_VIEW_HELPER_DELEGATE_H_
#define BRAVE_RENDERER_PRINTING_BRAVE_PRINT_WEB_VIEW_HELPER_DELEGATE_H_

#include "components/printing/renderer/print_web_view_helper.h"

class BravePrintWebViewHelperDelegate
    : public printing::PrintWebViewHelper::Delegate {
 public:
  ~BravePrintWebViewHelperDelegate() override;

  bool CancelPrerender(content::RenderFrame* render_frame) override;

  blink::WebElement GetPdfElement(blink::WebLocalFrame* frame) override;

  bool IsPrintPreviewEnabled() override;

  bool OverridePrint(blink::WebLocalFrame* frame) override;
};  // class BravePrintWebViewHelperDelegate

#endif  // BRAVE_RENDERER_PRINTING_BRAVE_PRINT_WEB_VIEW_HELPER_DELEGATE_H_
