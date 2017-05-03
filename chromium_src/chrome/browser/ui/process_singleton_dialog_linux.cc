// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/process_singleton_dialog_linux.h"

#include "base/run_loop.h"

bool ShowProcessSingletonDialog(const base::string16& message,
                                const base::string16& relaunch_text) {
  // Avoids gpu_process_transport_factory.cc(153)] Check failed:
  // per_compositor_data_.empty() when quit is chosen.
  base::RunLoop().RunUntilIdle();

  return false;
}
