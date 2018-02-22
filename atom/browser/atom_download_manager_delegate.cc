// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_download_manager_delegate.h"

#include <string>
#include <vector>

#include "atom/browser/native_window.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "chrome/browser/extensions/api/file_system/file_entry_picker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "vendor/brightray/browser/inspectable_web_contents.h"

namespace atom {

namespace {

const DownloadPathReservationTracker::FilenameConflictAction
    kDefaultPlatformConflictAction = DownloadPathReservationTracker::UNIQUIFY;

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

bool AtomDownloadManagerDelegate::GetItemExtension(
    content::DownloadItem* item,
    base::FilePath::StringType* extension) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  api::DownloadItem* download = api::DownloadItem::FromWrappedClass(isolate,
                                                                    item);
  if (download && !download->GetMimeType().empty())
      return net::GetPreferredExtensionForMimeType(
            download->GetMimeType(), extension);
  return false;
}

void AtomDownloadManagerDelegate:: OnDownloadItemSelected(
    const content::DownloadTargetCallback& callback,
    api::DownloadItem* download_item,
    const std::vector<base::FilePath>& paths) {
  DCHECK(!paths.empty());
  // Remember the last selected download directory.
  Profile* profile = static_cast<Profile*>(
      download_manager_->GetBrowserContext());
  profile->GetPrefs()->SetFilePath(prefs::kDownloadDefaultDirectory,
                                   paths[0].DirName());
  if (download_item)
    download_item->SetSavePath(paths[0]);

  callback.Run(paths[0],
               content::DownloadItem::TARGET_DISPOSITION_PROMPT,
               content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, paths[0],
               content::DOWNLOAD_INTERRUPT_REASON_NONE);
}

void AtomDownloadManagerDelegate::OnDownloadItemSelectionCancelled(
    const content::DownloadTargetCallback& callback,
    content::DownloadItem* item) {
  item->Remove();
  base::FilePath path;
  callback.Run(path,
               content::DownloadItem::TARGET_DISPOSITION_PROMPT,
               content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, path,
               content::DOWNLOAD_INTERRUPT_REASON_USER_CANCELED);
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

  NativeWindow* window = nullptr;
  content::WebContents* web_contents = item->GetWebContents();
  auto relay = web_contents ? NativeWindowRelay::FromWebContents(web_contents)
                            : nullptr;
  if (relay)
    window = relay->window.get();

  GetItemSavePath(item, &path);

  // Show save dialog if save path was not set already on item
  ui::SelectFileDialog::FileTypeInfo file_type_info;
  if (path.empty()) {
    std::vector<base::FilePath::StringType> extensions;
    base::FilePath::StringType extension;
    if (!GetItemExtension(item, &extension)) {
      extension = target_path.Extension();
      if (!extension.empty())
        extension.erase(extension.begin());  // Erase preceding '.'.
    }
    if (!extension.empty()) {
      extensions.push_back(extension);
      file_type_info.extensions.push_back(extensions);
    }
    file_type_info.include_all_files = true;
    new extensions::FileEntryPicker(
      window->inspectable_web_contents()->GetWebContents(),
      target_path,
      file_type_info,
      ui::SelectFileDialog::SELECT_SAVEAS_FILE,
      base::Bind(&AtomDownloadManagerDelegate::OnDownloadItemSelected,
                 base::Unretained(this), callback, download_item),
      base::Bind(&AtomDownloadManagerDelegate::OnDownloadItemSelectionCancelled,
                 base::Unretained(this), callback, item));
  } else {
    if (download_item)
      download_item->SetSavePath(path);

    callback.Run(path,
                 content::DownloadItem::TARGET_DISPOSITION_PROMPT,
                 content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, path,
                 content::DOWNLOAD_INTERRUPT_REASON_NONE);
  }
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
