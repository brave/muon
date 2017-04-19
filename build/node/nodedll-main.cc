// Copyright 2011 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#undef USING_V8_SHARED
#include "v8.h"
#include "env.h"
#include "env-inl.h"
#include "node.h"
#include "node_buffer.h"
#include "node_internals.h"

#if V8_OS_WIN
#include "src/base/win32-headers.h"

extern "C" {
  BOOL WINAPI DllMain(HANDLE hinstDLL,
    DWORD dwReason,
    LPVOID lpvReserved) {
    // Do nothing.
    return TRUE;
  }
}
#endif  // V8_OS_WIN
