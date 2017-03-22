// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/app/atom_main.h"

#include <stdlib.h>

#if defined(OS_WIN)
#include <windows.h>  // windows.h must be included first

#include <shellapi.h>
#include <shellscalingapi.h>
#include <tchar.h>

#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#include "base/debug/dump_without_crashing.h"
#include "base/win/win_util.h"
#include "chrome/install_static/product_install_details.h"
#endif  // defined(OS_WIN)

#if defined(OS_MACOSX)
#include "atom/app/atom_library_main.h"
#endif

#include "atom/app/atom_main_delegate.h"
#include "base/at_exit.h"
#include "content/public/app/content_main.h"

#if defined(OS_WIN)
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmd, int) {
#else
int main(int argc, const char* argv[]) {
#endif
#if defined(OS_MACOSX)
  return ChromeMain(argc, argv);
#else  // defined(OS_MACOSX)
  atom::AtomMainDelegate delegate;
  content::ContentMainParams params(&delegate);

#if defined(OS_WIN)
  install_static::InitializeProductDetailsForPrimaryModule();

  base::win::SetShouldCrashOnProcessDetach(true);
  base::win::SetAbortBehaviorForCrashReporting();

  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);

  params.instance = instance;
  params.sandbox_info = &sandbox_info;
#else
  params.argc = argc;
  params.argv = argv;
#endif

  base::AtExitManager exit_manager;

  return content::ContentMain(params);
#endif
}
