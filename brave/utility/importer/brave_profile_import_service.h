// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef BRAVE_UTILITY_BRAVE_PROFILE_IMPORT_SERVICE_H_
#define BRAVE_UTILITY_BRAVE_PROFILE_IMPORT_SERVICE_H_

#include <memory>

#include "chrome/utility/importer/profile_import_service.h"

class BraveProfileImportService : public ProfileImportService {
 public:
  BraveProfileImportService();
  ~BraveProfileImportService() override;

  // Factory method for creating the service.
  static std::unique_ptr<service_manager::Service> CreateService();

  DISALLOW_COPY_AND_ASSIGN(BraveProfileImportService);
};

#endif  // BRAVE_UTILITY_BRAVE_PROFILE_IMPORT_SERVICE_H_
