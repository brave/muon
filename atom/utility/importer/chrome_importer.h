// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_UTILITY_IMPORTER_CHROME_IMPORTER_H_
#define ATOM_UTILITY_IMPORTER_CHROME_IMPORTER_H_

#include <stdint.h>

#include <vector>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/utility/importer/importer.h"

struct ImportedBookmarkEntry;

namespace base {
class DictionaryValue;
}

class ChromeImporter : public Importer {
 public:
  ChromeImporter();

  // Importer:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t items,
                   ImporterBridge* bridge) override;

 private:
  ~ChromeImporter() override;

  void ImportBookmarks();
  void ImportHistory();
  void ImportCookies();

  void RecursiveReadBookmarksFolder(
    const base::DictionaryValue* folder,
    const std::vector<base::string16>& parent_path,
    bool is_in_toolbar,
    std::vector<ImportedBookmarkEntry>* bookmarks);

  double chromeTimeToDouble(int64_t time);

  base::FilePath source_path_;

  DISALLOW_COPY_AND_ASSIGN(ChromeImporter);
};

#endif  // ATOM_UTILITY_IMPORTER_CHROME_IMPORTER_H_
