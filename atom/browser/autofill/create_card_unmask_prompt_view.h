// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_AUTOFILL_CREATE_CARD_UNMASK_PROMPT_VIEW_H_
#define ATOM_BROWSER_AUTOFILL_CREATE_CARD_UNMASK_PROMPT_VIEW_H_

namespace content {
class WebContents;
}

namespace autofill {

class CardUnmaskPromptController;
class CardUnmaskPromptView;

// Factory function for CardUnmaskPromptView on non-iOS platforms.
CardUnmaskPromptView* CreateCardUnmaskPromptView(
    CardUnmaskPromptController* controller, content::WebContents* web_contents);

}  // namespace autofill

#endif  // ATOM_BROWSER_AUTOFILL_CREATE_CARD_UNMASK_PROMPT_VIEW_H_
