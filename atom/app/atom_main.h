// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_MAIN_H_
#define ATOM_APP_ATOM_MAIN_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <crtdbg.h>
struct AssertionDialogAvoider {
  AssertionDialogAvoider() {
    _CrtSetReportMode(_CRT_ASSERT, 0);
  }
};

const AssertionDialogAvoider AssertionDialogAvoider{};
#endif

#if defined(OS_MACOSX)
extern "C" {
__attribute__((visibility("default")))
int ChromeMain(int argc, const char* argv[]);
}
#endif  // OS_MACOSX

#endif  // ATOM_APP_ATOM_MAIN_H_
