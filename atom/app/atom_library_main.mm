// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_library_main.h"

#include "atom/app/atom_main_delegate.h"
#include "base/at_exit.h"
#include "content/public/app/content_main.h"

extern "C" {
__attribute__((visibility("default")))
int ChromeMain(int argc, const char** argv);
}

int ChromeMain(int argc, const char* argv[]) {
  base::AtExitManager exit_manager;

  atom::AtomMainDelegate delegate;
  content::ContentMainParams params(&delegate);
  params.argc = argc;
  params.argv = argv;
  return content::ContentMain(params);
}
