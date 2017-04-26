// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/importer/in_process_importer_bridge.h"

#include "atom/browser/importer/profile_writer.h"
#include "brave/common/importer/imported_cookie_entry.h"

namespace atom {

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


InProcessImporterBridge::InProcessImporterBridge(
    ::ProfileWriter* writer,
    base::WeakPtr<ExternalProcessImporterHost> host) :
  ::InProcessImporterBridge(writer, host),
  writer_(static_cast<atom::ProfileWriter*>(writer)) {
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

void InProcessImporterBridge::SetCookies(
    const std::vector<ImportedCookieEntry>& cookies) {
  writer_->AddCookies(cookies);
}

InProcessImporterBridge::~InProcessImporterBridge() {}

}  // namespace atom
