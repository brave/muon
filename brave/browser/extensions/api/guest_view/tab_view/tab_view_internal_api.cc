// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/extensions/api/guest_view/tab_view/tab_view_internal_api.h"

#include <string>

#include "atom/browser/extensions/tab_helper.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/guest_view/web_view/web_view_constants.h"
#include "extensions/browser/guest_view/web_view/web_view_content_script_manager.h"
#include "extensions/common/error_utils.h"

bool TabViewInternalExtensionFunction::PreRunValidation(std::string* error) {
  if (!UIThreadExtensionFunction::PreRunValidation(error))
    return false;

  int instance_id = 0;
  EXTENSION_FUNCTION_PRERUN_VALIDATE(args_->GetInteger(0, &instance_id));
  guest_ = brave::TabViewGuest::From(render_frame_host()->GetProcess()->GetID(),
                              instance_id);
  if (!guest_) {
    *error = "Could not find guest";
    return false;
  }
  return true;
}

TabViewInternalGetTabIDFunction::TabViewInternalGetTabIDFunction() {
}

TabViewInternalGetTabIDFunction::~TabViewInternalGetTabIDFunction() {
}

ExtensionFunction::ResponseAction TabViewInternalGetTabIDFunction::Run() {
  int tab_id = extensions::TabHelper::IdForTab(guest_->web_contents());
  return RespondNow(
      OneArgument(base::MakeUnique<base::FundamentalValue>(tab_id)));
}
