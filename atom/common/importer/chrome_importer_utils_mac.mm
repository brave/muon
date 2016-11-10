// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <Cocoa/Cocoa.h>
#include <sys/param.h>

#include "atom/common/importer/chrome_importer_utils.h"

#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"

base::FilePath GetChromeUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
 return result.Append("Application Support/Google/Chrome");
}

base::FilePath GetCanaryUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
 return result.Append("Application Support/Google/Chrome Canary");
}

base::FilePath GetChromiumUserDataFolder() {
  base::FilePath result = base::mac::GetUserLibraryPath();
 return result.Append("Application Support/Chromium");
}
