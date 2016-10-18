// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_BRAVE_PATHS_H__
#define BRAVE_COMMON_BRAVE_PATHS_H__

#include "build/build_config.h"

namespace base {
class FilePath;
}

namespace brave {

bool GetDefaultAppDataDirectory(base::FilePath* result);
bool GetDefaultUserDataDirectory(base::FilePath* path);

}  // namespace brave

#endif  // BRAVE_COMMON_BRAVE_PATHS_H__
