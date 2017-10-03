// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_

#include "components/keyed_service/core/keyed_service.h"

namespace feature_engagement {

class NewTabTracker : public KeyedService {
 public:
  NewTabTracker() = default;

  void OnNewTabOpened() {}

 protected:
  ~NewTabTracker() override = default;

  DISALLOW_COPY_AND_ASSIGN(NewTabTracker);
};

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_NEW_TAB_NEW_TAB_TRACKER_H_
