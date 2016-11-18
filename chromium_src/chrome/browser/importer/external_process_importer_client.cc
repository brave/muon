// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * This is not a straight copy from chromium src, in particular
 * we add SetCookies.
 * This was originally forked with 52.0.2743.116.  Diff against
 * a version of that file for a full list of changes.
 */

#include "chrome/browser/importer/external_process_importer_client.h"

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/importer/external_process_importer_host.h"
#include "chrome/browser/importer/in_process_importer_bridge.h"
#include "chrome/common/importer/firefox_importer_utils.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "chrome/common/importer/profile_import_process_messages.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/utility_process_host.h"
#include "grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

using content::BrowserThread;
using content::UtilityProcessHost;

ExternalProcessImporterClient::ExternalProcessImporterClient(
    base::WeakPtr<ExternalProcessImporterHost> importer_host,
    const importer::SourceProfile& source_profile,
    uint16_t items,
    InProcessImporterBridge* bridge)
    : total_bookmarks_count_(0),
      total_history_rows_count_(0),
      total_favicons_count_(0),
      process_importer_host_(importer_host),
      source_profile_(source_profile),
      items_(items),
      bridge_(bridge),
      cancelled_(false) {
  process_importer_host_->NotifyImportStarted();
}

void ExternalProcessImporterClient::Start() {
  AddRef();  // balanced in Cleanup.
  BrowserThread::ID thread_id;
  CHECK(BrowserThread::GetCurrentThreadIdentifier(&thread_id));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ExternalProcessImporterClient::StartProcessOnIOThread,
                 this,
                 thread_id));
}

void ExternalProcessImporterClient::Cancel() {
  if (cancelled_)
    return;

  cancelled_ = true;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(
          &ExternalProcessImporterClient::CancelImportProcessOnIOThread,
          this));
  Release();
}

void ExternalProcessImporterClient::OnProcessCrashed(int exit_code) {
  DLOG(ERROR) << __func__;
  if (cancelled_)
    return;

  // If the host is still around, cancel the import; otherwise it means the
  // import was already cancelled or completed and this message can be dropped.
  if (process_importer_host_.get())
    process_importer_host_->Cancel();
}

bool ExternalProcessImporterClient::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ExternalProcessImporterClient, message)
    // Notification messages about the state of the import process.
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_Import_Started,
                        OnImportStart)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_Import_Finished,
                        OnImportFinished)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_ImportItem_Started,
                        OnImportItemStart)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_ImportItem_Finished,
                        OnImportItemFinished)
    // Data messages containing items to be written to the user profile.
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyHistoryImportStart,
                        OnHistoryImportStart)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyHistoryImportGroup,
                        OnHistoryImportGroup)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyHomePageImportReady,
                        OnHomePageImportReady)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyBookmarksImportStart,
                        OnBookmarksImportStart)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyBookmarksImportGroup,
                        OnBookmarksImportGroup)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyFaviconsImportStart,
                        OnFaviconsImportStart)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyFaviconsImportGroup,
                        OnFaviconsImportGroup)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyPasswordFormReady,
                        OnPasswordFormImportReady)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyKeywordsReady,
                        OnKeywordsImportReady)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyFirefoxSearchEngData,
                        OnFirefoxSearchEngineDataReceived)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_AutofillFormDataImportStart,
                        OnAutofillFormDataImportStart)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_AutofillFormDataImportGroup,
                        OnAutofillFormDataImportGroup)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyCookiesImportStart,
                        OnCookiesImportStart)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyCookiesImportGroup,
                        OnCookiesImportGroup)
#if defined(OS_WIN)
    IPC_MESSAGE_HANDLER(ProfileImportProcessHostMsg_NotifyIE7PasswordInfo,
                        OnIE7PasswordReceived)
#endif
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ExternalProcessImporterClient::OnImportStart() {
  if (cancelled_)
    return;

  bridge_->NotifyStarted();
}

void ExternalProcessImporterClient::OnImportFinished(
    bool succeeded, const std::string& error_msg) {
  if (cancelled_)
    return;

  if (!succeeded)
    LOG(WARNING) << "Import failed.  Error: " << error_msg;
  Cleanup();
}

void ExternalProcessImporterClient::OnImportItemStart(int item_data) {
  if (cancelled_)
    return;

  bridge_->NotifyItemStarted(static_cast<importer::ImportItem>(item_data));
}

void ExternalProcessImporterClient::OnImportItemFinished(int item_data) {
  if (cancelled_)
    return;

  importer::ImportItem import_item =
      static_cast<importer::ImportItem>(item_data);
  bridge_->NotifyItemEnded(import_item);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ExternalProcessImporterClient::NotifyItemFinishedOnIOThread,
                 this,
                 import_item));
}

void ExternalProcessImporterClient::OnHistoryImportStart(
    size_t total_history_rows_count) {
  if (cancelled_)
    return;

  total_history_rows_count_ = total_history_rows_count;
  history_rows_.reserve(total_history_rows_count);
}

void ExternalProcessImporterClient::OnHistoryImportGroup(
    const std::vector<ImporterURLRow>& history_rows_group,
    int visit_source) {
  if (cancelled_)
    return;

  history_rows_.insert(history_rows_.end(), history_rows_group.begin(),
                       history_rows_group.end());
  if (history_rows_.size() >= total_history_rows_count_)
    bridge_->SetHistoryItems(history_rows_,
                             static_cast<importer::VisitSource>(visit_source));
}

void ExternalProcessImporterClient::OnHomePageImportReady(
    const GURL& home_page) {
  if (cancelled_)
    return;

  bridge_->AddHomePage(home_page);
}

void ExternalProcessImporterClient::OnBookmarksImportStart(
    const base::string16& first_folder_name,
    size_t total_bookmarks_count) {
  if (cancelled_)
    return;

  bookmarks_first_folder_name_ = first_folder_name;
  total_bookmarks_count_ = total_bookmarks_count;
  bookmarks_.reserve(total_bookmarks_count);
}

void ExternalProcessImporterClient::OnBookmarksImportGroup(
    const std::vector<ImportedBookmarkEntry>& bookmarks_group) {
  if (cancelled_)
    return;

  // Collect sets of bookmarks from importer process until we have reached
  // total_bookmarks_count_:
  bookmarks_.insert(bookmarks_.end(), bookmarks_group.begin(),
                    bookmarks_group.end());
  if (bookmarks_.size() >= total_bookmarks_count_)
    bridge_->AddBookmarks(bookmarks_, bookmarks_first_folder_name_);
}

void ExternalProcessImporterClient::OnFaviconsImportStart(
    size_t total_favicons_count) {
  if (cancelled_)
    return;

  total_favicons_count_ = total_favicons_count;
  favicons_.reserve(total_favicons_count);
}

void ExternalProcessImporterClient::OnFaviconsImportGroup(
    const favicon_base::FaviconUsageDataList& favicons_group) {
  if (cancelled_)
    return;

  favicons_.insert(favicons_.end(), favicons_group.begin(),
                    favicons_group.end());
  if (favicons_.size() >= total_favicons_count_)
    bridge_->SetFavicons(favicons_);
}

void ExternalProcessImporterClient::OnPasswordFormImportReady(
    const autofill::PasswordForm& form) {
  if (cancelled_)
    return;

  bridge_->SetPasswordForm(form);
}

void ExternalProcessImporterClient::OnKeywordsImportReady(
    const std::vector<importer::SearchEngineInfo>& search_engines,
    bool unique_on_host_and_path) {
  if (cancelled_)
    return;
  bridge_->SetKeywords(search_engines, unique_on_host_and_path);
}

void ExternalProcessImporterClient::OnFirefoxSearchEngineDataReceived(
    const std::vector<std::string> search_engine_data) {
  if (cancelled_)
    return;
  bridge_->SetFirefoxSearchEnginesXMLData(search_engine_data);
}

void ExternalProcessImporterClient::OnAutofillFormDataImportStart(
    size_t total_autofill_form_data_entry_count) {
  if (cancelled_)
    return;

  total_autofill_form_data_entry_count_ = total_autofill_form_data_entry_count;
  autofill_form_data_.reserve(total_autofill_form_data_entry_count);
}

void ExternalProcessImporterClient::OnAutofillFormDataImportGroup(
    const std::vector<ImporterAutofillFormDataEntry>&
        autofill_form_data_entry_group) {
  if (cancelled_)
    return;

  autofill_form_data_.insert(autofill_form_data_.end(),
                             autofill_form_data_entry_group.begin(),
                             autofill_form_data_entry_group.end());
  if (autofill_form_data_.size() >= total_autofill_form_data_entry_count_)
    bridge_->SetAutofillFormData(autofill_form_data_);
}
void ExternalProcessImporterClient::OnCookiesImportStart(
    size_t total_cookies_count) {
  if (cancelled_)
    return;

  total_cookies_count_ = total_cookies_count;
  cookies_.reserve(total_cookies_count);
}
void ExternalProcessImporterClient::OnCookiesImportGroup(
    const std::vector<ImportedCookieEntry>& cookies_group) {
  if (cancelled_)
    return;

  cookies_.insert(cookies_.end(), cookies_group.begin(),
                    cookies_group.end());
  if (cookies_.size() >= total_cookies_count_)
    bridge_->SetCookies(cookies_);
}

#if defined(OS_WIN)
void ExternalProcessImporterClient::OnIE7PasswordReceived(
    const importer::ImporterIE7PasswordInfo& importer_password_info) {
  if (cancelled_)
    return;
  bridge_->AddIE7PasswordInfo(importer_password_info);
}
#endif

ExternalProcessImporterClient::~ExternalProcessImporterClient() {}

void ExternalProcessImporterClient::Cleanup() {
  if (cancelled_)
    return;

  if (process_importer_host_.get())
    process_importer_host_->NotifyImportEnded();
  Release();
}

void ExternalProcessImporterClient::CancelImportProcessOnIOThread() {
  if (utility_process_host_.get())
    utility_process_host_->Send(new ProfileImportProcessMsg_CancelImport());
}

void ExternalProcessImporterClient::NotifyItemFinishedOnIOThread(
    importer::ImportItem import_item) {
  utility_process_host_->Send(
      new ProfileImportProcessMsg_ReportImportItemFinished(import_item));
}

void ExternalProcessImporterClient::StartProcessOnIOThread(
    BrowserThread::ID thread_id) {
  utility_process_host_ =
      UtilityProcessHost::Create(
          this, BrowserThread::GetTaskRunnerForThread(thread_id).get())
          ->AsWeakPtr();
  utility_process_host_->SetName(l10n_util::GetStringUTF16(
      IDS_UTILITY_PROCESS_PROFILE_IMPORTER_NAME));
  utility_process_host_->DisableSandbox();

#if defined(OS_MACOSX)
  base::EnvironmentMap env;
  std::string dylib_path = GetFirefoxDylibPath().value();
  if (!dylib_path.empty())
    env["DYLD_FALLBACK_LIBRARY_PATH"] = dylib_path;
  utility_process_host_->SetEnv(env);
#endif

  // Dictionary of all localized strings that could be needed by the importer
  // in the external process.
  base::DictionaryValue localized_strings;
  localized_strings.SetString(
      base::IntToString(IDS_BOOKMARK_GROUP),
      l10n_util::GetStringUTF8(IDS_BOOKMARK_GROUP));
  localized_strings.SetString(
      base::IntToString(IDS_BOOKMARK_GROUP_FROM_FIREFOX),
      l10n_util::GetStringUTF8(IDS_BOOKMARK_GROUP_FROM_FIREFOX));
  localized_strings.SetString(
      base::IntToString(IDS_BOOKMARK_GROUP_FROM_SAFARI),
      l10n_util::GetStringUTF8(IDS_BOOKMARK_GROUP_FROM_SAFARI));
  localized_strings.SetString(
      base::IntToString(IDS_IMPORT_FROM_FIREFOX),
      l10n_util::GetStringUTF8(IDS_IMPORT_FROM_FIREFOX));
  localized_strings.SetString(
      base::IntToString(IDS_IMPORT_FROM_ICEWEASEL),
      l10n_util::GetStringUTF8(IDS_IMPORT_FROM_ICEWEASEL));
  localized_strings.SetString(
      base::IntToString(IDS_IMPORT_FROM_SAFARI),
      l10n_util::GetStringUTF8(IDS_IMPORT_FROM_SAFARI));
  localized_strings.SetString(
      base::IntToString(IDS_BOOKMARK_BAR_FOLDER_NAME),
      l10n_util::GetStringUTF8(IDS_BOOKMARK_BAR_FOLDER_NAME));

  utility_process_host_->Send(new ProfileImportProcessMsg_StartImport(
      source_profile_, items_, localized_strings));
}
