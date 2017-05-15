// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/utility/importer/importer_creator.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "brave/utility/importer/chrome_importer.h"
#include "chrome/utility/importer/bookmarks_file_importer.h"
#include "brave/utility/importer/firefox_importer.h"

#if defined(OS_WIN)
#include "chrome/common/importer/edge_importer_utils_win.h"
#include "chrome/utility/importer/edge_importer_win.h"
#include "chrome/utility/importer/ie_importer_win.h"
#endif

#if defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>

#include "base/mac/foundation_util.h"
#include "chrome/utility/importer/safari_importer.h"
#endif

namespace brave_importer {

scoped_refptr<Importer> CreateImporterByType(importer::ImporterType type) {
  switch (type) {
#if defined(OS_WIN)
    case importer::TYPE_IE:
      return new IEImporter();
    case importer::TYPE_EDGE:
      // If legacy mode we pass back an IE importer.
      if (importer::IsEdgeFavoritesLegacyMode())
        return new IEImporter();
      return new EdgeImporter();
#endif
    case importer::TYPE_BOOKMARKS_FILE:
      return new BookmarksFileImporter();
    case importer::TYPE_FIREFOX:
      return new brave::FirefoxImporter();
#if defined(OS_MACOSX)
    case importer::TYPE_SAFARI:
      return new SafariImporter(base::mac::GetUserLibraryPath());
#endif
    case importer::TYPE_CHROME:
      return new ChromeImporter();
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace brave_importer
