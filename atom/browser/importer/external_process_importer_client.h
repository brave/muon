// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
#define ATOM_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_

#include <vector>

#include "chrome/browser/importer/external_process_importer_client.h"

#include "brave/common/importer/imported_cookie_entry.h"

namespace atom {

class InProcessImporterBridge;

class ExternalProcessImporterClient : public ::ExternalProcessImporterClient {
 public:
  ExternalProcessImporterClient(
      base::WeakPtr<ExternalProcessImporterHost> importer_host,
      const importer::SourceProfile& source_profile,
      uint16_t items,
      InProcessImporterBridge* bridge);

  // Called by the ExternalProcessImporterHost on import cancel.
  void Cancel();

  void OnCookiesImportStart(
      uint32_t total_cookies_count);
  void OnCookiesImportGroup(
      const std::vector<ImportedCookieEntry>&
          cookies_group);

 private:
  ~ExternalProcessImporterClient() override;

  // Total number of cookies to import.
  size_t total_cookies_count_;

  scoped_refptr<InProcessImporterBridge> bridge_;

  std::vector<ImportedCookieEntry> cookies_;

  // True if import process has been cancelled.
  bool cancelled_;

  DISALLOW_COPY_AND_ASSIGN(ExternalProcessImporterClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_IMPORTER_EXTERNAL_PROCESS_IMPORTER_CLIENT_H_
