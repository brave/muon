// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_IMPORTER_PROFILE_WRITER_H_
#define ATOM_BROWSER_IMPORTER_PROFILE_WRITER_H_

#include <vector>

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/importer/profile_writer.h"

struct ImportedCookieEntry;

namespace atom {

namespace api {
class Importer;
}

class ProfileWriter : public ::ProfileWriter {
 public:
  explicit ProfileWriter(Profile* profile);

  bool BookmarkModelIsLoaded() const override;
  bool TemplateURLServiceIsLoaded() const override;
  void AddPasswordForm(const autofill::PasswordForm& form) override;
#if defined(OS_WIN)
  void AddIE7PasswordInfo(const IE7PasswordInfo& info) override;
#endif
  void AddHistoryPage(const history::URLRows& page,
                              history::VisitSource visit_source) override;
  void AddHomepage(const GURL& homepage) override;
  void AddKeywords(
      TemplateURLService::OwnedTemplateURLVector template_urls,
      bool unique_on_host_and_path) override {};
  void AddBookmarks(
      const std::vector<ImportedBookmarkEntry>& bookmarks,
      const base::string16& top_level_folder_name) override;
  void AddFavicons(const favicon_base::FaviconUsageDataList& favicons) override;

  // Adds the imported autofill entries to the autofill database.
  void AddAutofillFormDataEntries(
      const std::vector<autofill::AutofillEntry>& autofill_entries) override;
  virtual void AddCookies(const std::vector<ImportedCookieEntry>& cookies);
  void Initialize(atom::api::Importer* importer);

  bool ShowWarningDialog();

 protected:
  friend class base::RefCountedThreadSafe<ProfileWriter>;

  virtual ~ProfileWriter();

 private:
  // Importer instance of Brave
  atom::api::Importer* importer_;

  DISALLOW_COPY_AND_ASSIGN(ProfileWriter);
};

}  // namespace atom

#endif  // ATOM_BROWSER_IMPORTER_PROFILE_WRITER_H_
