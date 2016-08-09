// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_AUTOFILL_RISK_UTIL_H_
#define ATOM_BROWSER_AUTOFILL_RISK_UTIL_H_

#include <stdint.h>

#include <string>

#include "base/callback_forward.h"

namespace content {
class WebContents;
}

namespace autofill {

void LoadRiskData(uint64_t obfuscated_gaia_id,
                  content::WebContents* web_contents,
                  const base::Callback<void(const std::string&)>& callback);

}  // namespace autofill

#endif  // ATOM_BROWSER_AUTOFILL_RISK_UTIL_H_
