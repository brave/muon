// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The entry point for all Mac Chromium processes, including the outer app
// bundle (browser) and helper app (renderer, plugin, and friends).

#include <dlfcn.h>
#include <errno.h>
#include <libgen.h>
#include <mach-o/dyld.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "chrome/common/chrome_version.h"

typedef int (*ChromeMainPtr)(int, char**);

__attribute__((visibility("default"))) int main(int argc, char* argv[]) {
#if defined(HELPER_EXECUTABLE)
  const char* const rel_path =
      "../../../" PRODUCT_FULLNAME_STRING
      " Framework.framework/" PRODUCT_FULLNAME_STRING " Framework";
#else
  const char* const rel_path =
      "../Frameworks/" PRODUCT_FULLNAME_STRING
      " Framework.framework/" PRODUCT_FULLNAME_STRING " Framework";
#endif  // defined(HELPER_EXECUTABLE)

  uint32_t exec_path_size = 0;
  int rv = _NSGetExecutablePath(NULL, &exec_path_size);
  if (rv != -1) {
    fprintf(stderr, "_NSGetExecutablePath: get length failed\n");
    abort();
  }

  char* exec_path = malloc(exec_path_size);
  if (!exec_path) {
    fprintf(stderr, "malloc %u: %s\n", exec_path_size, strerror(errno));
    abort();
  }

  rv = _NSGetExecutablePath(exec_path, &exec_path_size);
  if (rv != 0) {
    fprintf(stderr, "_NSGetExecutablePath: get path failed\n");
    abort();
  }

  // Slice off the last part of the main executable path, and append the
  // version framework information.
  const char* parent_dir = dirname(exec_path);
  if (!parent_dir) {
    fprintf(stderr, "dirname %s: %s\n", exec_path, strerror(errno));
    abort();
  }
  free(exec_path);

  const size_t parent_path_len = strlen(parent_dir);
  const size_t rel_path_len = strlen(rel_path);
  // 2 accounts for a trailing NUL byte and the '/' in the middle of the paths.
  const size_t framework_path_size = parent_path_len + rel_path_len + 2;
  char* framework_path = malloc(framework_path_size);
  if (!framework_path) {
    fprintf(stderr, "malloc %zu: %s\n", framework_path_size, strerror(errno));
    abort();
  }
  snprintf(framework_path, framework_path_size, "%s/%s", parent_dir, rel_path);

  void* library = dlopen(framework_path, RTLD_LAZY | RTLD_GLOBAL | RTLD_FIRST);
  if (!library) {
    fprintf(stderr, "dlopen %s: %s\n", framework_path, dlerror());
    abort();
  }
  free(framework_path);

  const ChromeMainPtr chrome_main = dlsym(library, "ChromeMain");
  if (!chrome_main) {
    fprintf(stderr, "dlsym ChromeMain: %s\n", dlerror());
    abort();
  }
  rv = chrome_main(argc, argv);

  // exit, don't return from main, to avoid the apparent removal of main from
  // stack backtraces under tail call optimization.
  exit(rv);
}
