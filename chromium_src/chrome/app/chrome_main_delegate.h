// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_CHROME_MAIN_DELEGATE_H_
#define CHROME_APP_CHROME_MAIN_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "atom/app/atom_main_delegate.h"

class ChromeMainDelegate : public atom::AtomMainDelegate {
 public:
  ChromeMainDelegate();

  // |exe_entry_point_ticks| is the time at which the main function of the
  // executable was entered, or null if not available.
  explicit ChromeMainDelegate(base::TimeTicks exe_entry_point_ticks);
  ~ChromeMainDelegate() override;

  DISALLOW_COPY_AND_ASSIGN(ChromeMainDelegate);
};

#endif  // CHROME_APP_CHROME_MAIN_DELEGATE_H_
