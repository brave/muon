// Copyright 2015 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/simple_message_box.h"

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "ui/gfx/native_widget_types.h"

namespace chrome {

void ShowWarningMessageBox(gfx::NativeWindow parent,
                           const base::string16& title,
                           const base::string16& message) {
  // TODO(bridiver)
  // atom:Message...
}

MessageBoxResult ShowQuestionMessageBox(gfx::NativeWindow parent,
                                        const base::string16& title,
                                        const base::string16& message) {
  return MESSAGE_BOX_RESULT_YES;
}

}  // namespace chrome
