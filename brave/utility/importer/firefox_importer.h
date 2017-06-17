// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_
#define BRAVE_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_

#include "chrome/utility/importer/firefox_importer.h"

#include <string>

namespace brave {

class FirefoxImporter : public ::FirefoxImporter {
 public:
  FirefoxImporter();

  // Importer:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t items,
                   ImporterBridge* bridge) override;

 private:
  ~FirefoxImporter();

  void ImportCookies();
  void ImportSitePasswordPrefs();

  base::FilePath source_path_;

  DISALLOW_COPY_AND_ASSIGN(FirefoxImporter);
};

}  // namespace brave

#endif  // BRAVE_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_
