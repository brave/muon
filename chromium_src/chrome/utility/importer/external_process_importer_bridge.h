// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This is not a straight copy from chromium src, in particular
 * we add SetCookies.
 * This was originally forked with 52.0.2743.116.  Diff against
 * a version of that file for a full list of changes.
 */

#ifndef CHROME_UTILITY_IMPORTER_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_
#define CHROME_UTILITY_IMPORTER_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/common/importer/importer_bridge.h"
#include "components/favicon_base/favicon_usage_data.h"

class GURL;
struct ImportedBookmarkEntry;

namespace base {
class DictionaryValue;
class TaskRunner;
}

namespace importer {
#if defined(OS_WIN)
struct ImporterIE7PasswordInfo;
#endif
struct ImporterURLRow;
struct SearchEngineInfo;
}

namespace IPC {
class Message;
class Sender;
}

// When the importer is run in an external process, the bridge is effectively
// split in half by the IPC infrastructure.  The external bridge receives data
// and notifications from the importer, and sends it across IPC.  The
// internal bridge gathers the data from the IPC host and writes it to the
// profile.
class ExternalProcessImporterBridge : public ImporterBridge {
 public:
  ExternalProcessImporterBridge(
      const base::DictionaryValue& localized_strings,
      IPC::Sender* sender,
      base::TaskRunner* task_runner);

  // Begin ImporterBridge implementation:
  void AddBookmarks(const std::vector<ImportedBookmarkEntry>& bookmarks,
                    const base::string16& first_folder_name) override;

  void AddHomePage(const GURL& home_page) override;

#if defined(OS_WIN)
  void AddIE7PasswordInfo(
      const importer::ImporterIE7PasswordInfo& password_info) override;
#endif

  void SetFavicons(const favicon_base::FaviconUsageDataList& favicons) override;

  void SetHistoryItems(const std::vector<ImporterURLRow>& rows,
                       importer::VisitSource visit_source) override;

  void SetKeywords(
      const std::vector<importer::SearchEngineInfo>& search_engines,
      bool unique_on_host_and_path) override;

  void SetFirefoxSearchEnginesXMLData(
      const std::vector<std::string>& seach_engine_data) override;

  void SetPasswordForm(const autofill::PasswordForm& form) override;

  void SetAutofillFormData(
      const std::vector<ImporterAutofillFormDataEntry>& entries) override;

  void SetCookies(const std::vector<ImportedCookieEntry>& cookies) override;

  void NotifyStarted() override;
  void NotifyItemStarted(importer::ImportItem item) override;
  void NotifyItemEnded(importer::ImportItem item) override;
  void NotifyEnded() override;

  base::string16 GetLocalizedString(int message_id) override;
  // End ImporterBridge implementation.

 private:
  ~ExternalProcessImporterBridge() override;

  void Send(IPC::Message* message);
  void SendInternal(IPC::Message* message);

  // Holds strings needed by the external importer because the resource
  // bundle isn't available to the external process.
  std::unique_ptr<base::DictionaryValue> localized_strings_;

  IPC::Sender* sender_;
  scoped_refptr<base::TaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(ExternalProcessImporterBridge);
};

#endif  // CHROME_UTILITY_IMPORTER_EXTERNAL_PROCESS_IMPORTER_BRIDGE_H_
