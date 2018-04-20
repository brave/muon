// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/util/muon_distribution.h"

#include "chrome/installer/util/app_registration_data.h"
#include "chrome/installer/util/non_updating_app_registration_data.h"

MuonDistribution::MuonDistribution()
    : BrowserDistribution(std::make_unique<NonUpdatingAppRegistrationData>(
          L"Software\\Muon")) {}
