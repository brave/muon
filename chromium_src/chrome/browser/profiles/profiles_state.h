// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILES_STATE_H_
#define CHROME_BROWSER_PROFILES_PROFILES_STATE_H_

#include <string>
#include <vector>
#include "base/strings/string16.h"
#include "chrome/browser/profiles/avatar_menu.h"

class Browser;
class PrefRegistrySimple;
class Profile;
class SigninErrorController;
namespace base { class FilePath; }

namespace profiles {

// Checks if multiple profiles is enabled.
bool IsMultipleProfilesEnabled();

}  // namespace profiles

#endif  // CHROME_BROWSER_PROFILES_PROFILES_STATE_H_
