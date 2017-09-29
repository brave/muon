// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/utility/importer/brave_profile_import_service.h"

BraveProfileImportService::BraveProfileImportService()
    : ProfileImportService() {}

BraveProfileImportService::~BraveProfileImportService() {}

std::unique_ptr<service_manager::Service>
BraveProfileImportService::CreateService() {
  return base::MakeUnique<BraveProfileImportService>();
}
