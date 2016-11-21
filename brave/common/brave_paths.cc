// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/common/brave_paths.h"
#include "base/environment.h"
#include "base/lazy_instance.h"
#include "base/files/file_path.h"
#include "base/nix/xdg_util.h"
#include "base/path_service.h"
#include "base/files/file_path.h"
#include "browser/brightray_paths.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths_internal.h"
#include <memory>

namespace brave {
  bool GetDefaultAppDataDirectory(base::FilePath* result) {
    #if defined(OS_LINUX)
    std::unique_ptr<base::Environment> env(base::Environment::Create());
    if (!env)
      return false;
      
    *result = base::nix::GetXDGDirectory(env.get(), 
        base::nix::kXdgConfigHomeEnvVar,
        base::nix::kDotConfigDir);
    if (result->BaseName().value() != base::nix::kDotConfigDir)
      return false;
    return true;
    #else
      return PathService::Get(brightray::DIR_APP_DATA, result);
    #endif
  }

  bool GetDefaultUserDataDirectory(base::FilePath* result) {
#if defined(OS_MACOSX)
    // on mac we SetOverrideVersionedDirectory to fix GetVersionedDirectory
    return chrome::GetDefaultUserDataDirectory(result);
#else
    if (!GetDefaultAppDataDirectory(result)) {
      return false;
    }
    result->Append(FILE_PATH_LITERAL("brave"));
    return true;
#endif
  }
}
