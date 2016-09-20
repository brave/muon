// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This is not a straight copy from chromium src, in particular
 * we add ImportCookie and fix broken ImportBookmarks.
 * This was originally forked with 52.0.2743.116.  Diff against
 * a version of that file for a full list of changes.
 */

#ifndef CHROME_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_
#define CHROME_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/utility/importer/importer.h"
#include "components/favicon_base/favicon_usage_data.h"

class GURL;

namespace sql {
class Connection;
}

// Importer for Mozilla Firefox 3 and later.
// Firefox stores its persistent information in a system called places.
// http://wiki.mozilla.org/Places
class FirefoxImporter : public Importer {
 public:
  FirefoxImporter();

  // Importer:
  void StartImport(const importer::SourceProfile& source_profile,
                   uint16_t items,
                   ImporterBridge* bridge) override;

 private:
  typedef std::map<int64_t, std::set<GURL>> FaviconMap;

  ~FirefoxImporter() override;

  void ImportBookmarks();
  void ImportPasswords();
  void ImportHistory();
  void ImportSearchEngines();
  // Import the user's home page, unless it is set to default home page as
  // defined in browserconfig.properties.
  void ImportHomepage();
  void ImportAutofillFormData();
  void ImportCookies();
  void GetSearchEnginesXMLData(std::vector<std::string>* search_engine_data);
  void GetSearchEnginesXMLDataFromJSON(
      std::vector<std::string>* search_engine_data);

  // The struct stores the information about a bookmark item.
  struct BookmarkItem;
  typedef std::vector<BookmarkItem*> BookmarkList;

  // Gets the specific IDs of bookmark root node from |db|.
  void LoadRootNodeID(sql::Connection* db, int* toolbar_folder_id,
                      int* menu_folder_id, int* unsorted_folder_id);

  // Loads all livemark IDs from database |db|.
  void LoadLivemarkIDs(sql::Connection* db, std::set<int>* livemark);

  // Gets the bookmark folder with given ID, and adds the entry in |list|
  // if successful.
  void GetTopBookmarkFolder(sql::Connection* db,
                            int folder_id,
                            BookmarkList* list);

  // Loads all children of the given folder, and appends them to the |list|.
  void GetWholeBookmarkFolder(sql::Connection* db, BookmarkList* list,
                              size_t position, bool* empty_folder);

  // Loads the favicons given in the map from the database, loads the data,
  // and converts it into FaviconUsage structures.
  void LoadFavicons(sql::Connection* db,
                    const FaviconMap& favicon_map,
                    favicon_base::FaviconUsageDataList* favicons);

  base::FilePath source_path_;
  base::FilePath app_path_;

#if defined(OS_POSIX)
  // Stored because we can only access it from the UI thread.
  std::string locale_;
#endif

  DISALLOW_COPY_AND_ASSIGN(FirefoxImporter);
};

#endif  // CHROME_UTILITY_IMPORTER_FIREFOX_IMPORTER_H_
