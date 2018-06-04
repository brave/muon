// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_
#define BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_

#include <string>
#include <memory>

#include "chrome/utility/importer/profile_import_impl.h"

class BraveProfileImportImpl : public ProfileImportImpl {
 public:
  explicit BraveProfileImportImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~BraveProfileImportImpl() override;

 private:
  // chrome::mojom::ProfileImport:
  void StartImport(
      const importer::SourceProfile& source_profile,
      uint16_t items,
      const base::flat_map<uint32_t, std::string>& localized_strings,
      chrome::mojom::ProfileImportObserverPtr observer) override;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileImportImpl);
};

#endif  // BRAVE_UTILITY_IMPORTER_BRAVE_PROFILE_IMPORT_IMPL_H_
