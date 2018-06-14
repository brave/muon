// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/renderer/printing/brave_print_render_frame_helper_delegate.h"

#include <vector>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "chrome/renderer/prerender/prerender_helper.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "ipc/ipc_message.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"

BravePrintRenderFrameHelperDelegate::~BravePrintRenderFrameHelperDelegate() {
}

bool BravePrintRenderFrameHelperDelegate::CancelPrerender(
    content::RenderFrame* render_frame) {
  return false;
}

// Return the PDF object element if |frame| is the out of process PDF extension.
blink::WebElement BravePrintRenderFrameHelperDelegate::GetPdfElement(
        blink::WebLocalFrame* frame) {
  return blink::WebElement();
}

bool BravePrintRenderFrameHelperDelegate::IsPrintPreviewEnabled() {
  return false;
}

bool BravePrintRenderFrameHelperDelegate::OverridePrint(
    blink::WebLocalFrame* frame) {
  return false;
}
