// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/importer/chrome_importer_utils.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string16.h"

base::FilePath GetChromeUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &result))
    return base::FilePath();

  result = result.AppendASCII("Google");
  result = result.AppendASCII("Chrome");
  result = result.AppendASCII("User Data");

  return result;
}

base::FilePath GetCanaryUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &result))
    return base::FilePath();

  result = result.AppendASCII("Google");
  result = result.AppendASCII("Chrome SxS");
  result = result.AppendASCII("User Data");

  return result;
}

base::FilePath GetChromiumUserDataFolder() {
  base::FilePath result;
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &result))
    return base::FilePath();

  result = result.AppendASCII("Chromium");
  result = result.AppendASCII("User Data");

  return result;
}
