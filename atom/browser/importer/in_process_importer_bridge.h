// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_IMPORTER_IN_PROCESS_IMPORTER_BRIDGE_H_
#define ATOM_BROWSER_IMPORTER_IN_PROCESS_IMPORTER_BRIDGE_H_

#include <string>
#include <vector>

#include "chrome/browser/importer/in_process_importer_bridge.h"

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"

struct ImportedCookieEntry;

namespace atom {

class ProfileWriter;

class InProcessImporterBridge : public ::InProcessImporterBridge {
 public:
  InProcessImporterBridge(::ProfileWriter* writer,
                          base::WeakPtr<ExternalProcessImporterHost> host);

  void SetHistoryItems(const std::vector<ImporterURLRow>& rows,
                       importer::VisitSource visit_source) override;

  void SetKeywords(
      const std::vector<importer::SearchEngineInfo>& search_engines,
      bool unique_on_host_and_path) override;

  void SetFirefoxSearchEnginesXMLData(
      const std::vector<std::string>& search_engine_data) override;

  virtual void SetCookies(const std::vector<ImportedCookieEntry>& cookies);

 private:
  ~InProcessImporterBridge() override;

  ProfileWriter* const writer_;  // weak

  DISALLOW_COPY_AND_ASSIGN(InProcessImporterBridge);
};

}  // namespace atom

#endif  // ATOM_BROWSER_IMPORTER_IN_PROCESS_IMPORTER_BRIDGE_H_
