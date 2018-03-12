// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_download_manager_delegate.h"

#include <string>

#include "atom/browser/api/atom_api_download_item.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "net/base/filename_util.h"

namespace atom {

namespace {

const DownloadPathReservationTracker::FilenameConflictAction
    kDefaultPlatformConflictAction = DownloadPathReservationTracker::UNIQUIFY;

NativeWindow* GetNativeWindowFromWebContents(content::WebContents* web_contents) {
  DCHECK(web_contents);
  auto relay = NativeWindowRelay::FromWebContents(web_contents);
  return relay ? relay->window.get() : nullptr;
}

// Blank newtab on download should be closed after starting download.
void CloseWebContentsIfNeeded(content::WebContents* web_contents) {
  DCHECK(web_contents);
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  web_contents->Close();
}

}  // namespace

AtomDownloadManagerDelegate::AtomDownloadManagerDelegate(
    content::DownloadManager* manager)
    : download_manager_(manager),
      weak_ptr_factory_(this) {}

AtomDownloadManagerDelegate::~AtomDownloadManagerDelegate() {
  if (download_manager_) {
    DCHECK_EQ(static_cast<content::DownloadManagerDelegate*>(this),
              download_manager_->GetDelegate());
    download_manager_->SetDelegate(nullptr);
    download_manager_ = nullptr;
  }
}

void AtomDownloadManagerDelegate::GetItemSavePath(content::DownloadItem* item,
                                                  base::FilePath* path) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  api::DownloadItem* download = api::DownloadItem::FromWrappedClass(isolate,
                                                                    item);
  if (download && !download->GetSavePath().empty())
    *path = download->GetSavePath();
}

void AtomDownloadManagerDelegate::OnDownloadPathGenerated(
    int32_t download_id,
    const content::DownloadTargetCallback& callback,
    PathValidationResult result,
    const base::FilePath& target_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto item = download_manager_->GetDownload(download_id);
  if (!item)
    return;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  api::DownloadItem* download_item = api::DownloadItem::FromWrappedClass(
      isolate, item);

  if (!download_item)
    download_item = atom::api::DownloadItem::Create(isolate, item).get();

  base::FilePath path;
  if (result == PathValidationResult::SUCCESS &&
      !download_item->ShouldPrompt()) {
    path = target_path;
  }

  GetItemSavePath(item, &path);

  // Show save dialog if save path was not set already on item
  file_dialog::DialogSettings settings;
  settings.parent_window = window;
  settings.title = item->GetURL().spec();
  settings.default_path = target_path;
  if (path.empty() && file_dialog::ShowSaveDialog(settings, &path)) {
    // Remember the last selected download directory.
    Profile* profile = static_cast<Profile*>(
        download_manager_->GetBrowserContext());
    profile->GetPrefs()->SetFilePath(prefs::kDownloadDefaultDirectory,
                                          path.DirName());
  }

  // To show file save dialog, |window| always must be valid.
  NativeWindow* window = nullptr;
  if (content::WebContents* web_contents = item->GetWebContents()) {
    window = GetNativeWindowFromWebContents(web_contents);
    CloseWebContentsIfNeeded(web_contents);
  }

  // If we can't find proper |window| for showing save dialog, cancel
  // and cleanup current download.
  if (!window) {
    OnDownloadItemSelectionCancelled(callback, item);
    return;
  }

  if (path.empty())
    item->Remove();

  if (download_item)
    download_item->SetSavePath(path);

  callback.Run(path,
               content::DownloadItem::TARGET_DISPOSITION_PROMPT,
               content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, path,
               path.empty()
                  ? content::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED
                  : content::DOWNLOAD_INTERRUPT_REASON_NONE);
}

void AtomDownloadManagerDelegate::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  download_manager_ = nullptr;
}

bool AtomDownloadManagerDelegate::DetermineDownloadTarget(
    content::DownloadItem* download,
    const content::DownloadTargetCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Profile* browser_context = static_cast<Profile*>(
      download_manager_->GetBrowserContext());
  base::FilePath default_download_path(browser_context->GetPrefs()->GetFilePath(
      prefs::kDownloadDefaultDirectory));

  DownloadPathReservationTracker::FilenameConflictAction conflict_action =
      DownloadPathReservationTracker::OVERWRITE;
  base::FilePath virtual_path = download->GetForcedFilePath();

  if (virtual_path.empty()) {
    std::string suggested_filename(download->GetSuggestedFilename());
    if (suggested_filename.empty() &&
        download->GetMimeType() == "application/x-x509-user-cert") {
      suggested_filename = "user.crt";
    }

    base::FilePath generated_filename = net::GenerateFileName(
        download->GetURL(),
        download->GetContentDisposition(),
        std::string(),
        suggested_filename,
        download->GetMimeType(),
        std::string());

    conflict_action = kDefaultPlatformConflictAction;
    virtual_path = default_download_path.Append(generated_filename);
  }

  DownloadPathReservationTracker::GetReservedPath(
      download,
      virtual_path,
      default_download_path,
      true,
      conflict_action,
      base::Bind(&AtomDownloadManagerDelegate::OnDownloadPathGenerated,
                 weak_ptr_factory_.GetWeakPtr(),
                 download->GetId(),
                 callback));
  return true;
}

bool AtomDownloadManagerDelegate::ShouldOpenDownload(
    content::DownloadItem* download,
    const content::DownloadOpenDelayedCallback& callback) {
  return true;
}

void AtomDownloadManagerDelegate::GetNextId(
    const content::DownloadIdCallback& callback) {
  static uint32_t next_id = content::DownloadItem::kInvalidId + 1;
  callback.Run(next_id++);
}

}  // namespace atom
