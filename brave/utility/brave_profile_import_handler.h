// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UTILITY_BRAVE_PROFILE_IMPORT_HANDLER_H_
#define BRAVE_UTILITY_BRAVE_PROFILE_IMPORT_HANDLER_H_

#include <memory>

#include "chrome/utility/profile_import_handler.h"

// Dispatches IPCs for out of process profile import.
class BraveProfileImportHandler : public ProfileImportHandler {
 public:
  BraveProfileImportHandler();
  ~BraveProfileImportHandler() override;

  static void Create(
      mojo::InterfaceRequest<chrome::mojom::ProfileImport> request);

 private:
  // chrome::mojom::ProfileImport:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t items,
                   std::unique_ptr<base::DictionaryValue> localized_strings,
                   chrome::mojom::ProfileImportObserverPtr observer) override;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileImportHandler);
};

#endif  // BRAVE_UTILITY_BRAVE_PROFILE_IMPORT_HANDLER_H_
