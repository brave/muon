// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/importer/in_process_importer_bridge.h"

#include <stddef.h>

#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/importer/imported_cookie_entry.h"
#include "build/build_config.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/search_engines/ui_thread_search_terms_data.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/importer_autofill_form_data_entry.h"
#include "components/autofill/core/browser/webdata/autofill_entry.h"
#include "components/autofill/core/common/password_form.h"
#include "components/favicon_base/favicon_usage_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_parser.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_WIN)
#include "components/os_crypt/ie7_password_win.h"
#endif

#include <iterator>

namespace {

history::URLRows ConvertImporterURLRowsToHistoryURLRows(
    const std::vector<ImporterURLRow>& rows) {
  history::URLRows converted;
  converted.reserve(rows.size());
  for (std::vector<ImporterURLRow>::const_iterator it = rows.begin();
       it != rows.end(); ++it) {
    history::URLRow row(it->url);
    row.set_title(it->title);
    row.set_visit_count(it->visit_count);
    row.set_typed_count(it->typed_count);
    row.set_last_visit(it->last_visit);
    row.set_hidden(it->hidden);
    converted.push_back(row);
  }
  return converted;
}

history::VisitSource ConvertImporterVisitSourceToHistoryVisitSource(
    importer::VisitSource visit_source) {
  switch (visit_source) {
    case importer::VISIT_SOURCE_BROWSED:
      return history::SOURCE_BROWSED;
    case importer::VISIT_SOURCE_FIREFOX_IMPORTED:
      return history::SOURCE_FIREFOX_IMPORTED;
    case importer::VISIT_SOURCE_IE_IMPORTED:
      return history::SOURCE_IE_IMPORTED;
    case importer::VISIT_SOURCE_SAFARI_IMPORTED:
      return history::SOURCE_SAFARI_IMPORTED;
    case importer::VISIT_SOURCE_CHROME_IMPORTED:
      // TODO(Anthony): pull components/history/core/browser/history_types.h
      return history::SOURCE_SYNCED;
  }
  NOTREACHED();
  return history::SOURCE_SYNCED;
}

}  // namespace

namespace {

// FirefoxURLParameterFilter is used to remove parameter mentioning Firefox from
// the search URL when importing search engines.
class FirefoxURLParameterFilter : public TemplateURLParser::ParameterFilter {
 public:
  FirefoxURLParameterFilter() {}
  ~FirefoxURLParameterFilter() override {}

  // TemplateURLParser::ParameterFilter method.
  bool KeepParameter(const std::string& key,
                     const std::string& value) override {
    std::string low_value = base::ToLowerASCII(value);
    if (low_value.find("mozilla") != std::string::npos ||
        low_value.find("firefox") != std::string::npos ||
        low_value.find("moz:") != std::string::npos) {
      return false;
    }
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FirefoxURLParameterFilter);
};

}  // namespace

InProcessImporterBridge::InProcessImporterBridge(
    ProfileWriter* writer,
    base::WeakPtr<ExternalProcessImporterHost> host) : writer_(writer),
                                                       host_(host) {
}

void InProcessImporterBridge::AddBookmarks(
    const std::vector<ImportedBookmarkEntry>& bookmarks,
    const base::string16& first_folder_name) {
  writer_->AddBookmarks(bookmarks, first_folder_name);
}

void InProcessImporterBridge::AddHomePage(const GURL& home_page) {
  writer_->AddHomepage(home_page);
}

#if defined(OS_WIN)
void InProcessImporterBridge::AddIE7PasswordInfo(
    const importer::ImporterIE7PasswordInfo& password_info) {
  IE7PasswordInfo ie7_password_info;
  ie7_password_info.url_hash = password_info.url_hash;
  ie7_password_info.encrypted_data = password_info.encrypted_data;
  ie7_password_info.date_created = password_info.date_created;

  writer_->AddIE7PasswordInfo(ie7_password_info);
}
#endif  // OS_WIN

void InProcessImporterBridge::SetFavicons(
    const favicon_base::FaviconUsageDataList& favicons) {
  writer_->AddFavicons(favicons);
}

void InProcessImporterBridge::SetHistoryItems(
    const std::vector<ImporterURLRow>& rows,
    importer::VisitSource visit_source) {
  history::URLRows converted_rows =
      ConvertImporterURLRowsToHistoryURLRows(rows);
  history::VisitSource converted_visit_source =
      ConvertImporterVisitSourceToHistoryVisitSource(visit_source);
  writer_->AddHistoryPage(converted_rows, converted_visit_source);
}

void InProcessImporterBridge::SetKeywords(
    const std::vector<importer::SearchEngineInfo>& search_engines,
    bool unique_on_host_and_path) {
}

void InProcessImporterBridge::SetFirefoxSearchEnginesXMLData(
    const std::vector<std::string>& search_engine_data) {
}

void InProcessImporterBridge::SetPasswordForm(
    const autofill::PasswordForm& form) {
  writer_->AddPasswordForm(form);
}

void InProcessImporterBridge::SetAutofillFormData(
    const std::vector<ImporterAutofillFormDataEntry>& entries) {
  std::vector<autofill::AutofillEntry> autofill_entries;
  for (size_t i = 0; i < entries.size(); ++i) {
    autofill_entries.push_back(autofill::AutofillEntry(
        autofill::AutofillKey(entries[i].name, entries[i].value),
        entries[i].first_used,
        entries[i].last_used));
  }

  writer_->AddAutofillFormDataEntries(autofill_entries);
}

void InProcessImporterBridge::SetCookies(
    const std::vector<ImportedCookieEntry>& cookies) {
  writer_->AddCookies(cookies);
}

void InProcessImporterBridge::NotifyStarted() {
  host_->NotifyImportStarted();
}

void InProcessImporterBridge::NotifyItemStarted(importer::ImportItem item) {
  host_->NotifyImportItemStarted(item);
}

void InProcessImporterBridge::NotifyItemEnded(importer::ImportItem item) {
  host_->NotifyImportItemEnded(item);
}

void InProcessImporterBridge::NotifyEnded() {
  host_->NotifyImportEnded();
}

base::string16 InProcessImporterBridge::GetLocalizedString(int message_id) {
  return l10n_util::GetStringUTF16(message_id);
}

InProcessImporterBridge::~InProcessImporterBridge() {}
