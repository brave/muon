// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_DOWNLOAD_MANAGER_DELEGATE_H_
#define ATOM_BROWSER_ATOM_DOWNLOAD_MANAGER_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/browser/api/atom_api_download_item.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/download/download_path_reservation_tracker.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/download/download_target_determiner.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_service.h"
#include "chrome/browser/safe_browsing/download_protection/download_protection_util.h"
#include "content/public/browser/download_manager_delegate.h"

class DownloadPrefs;
class Profile;

namespace content {
class DownloadManager;
}

namespace atom {

class AtomDownloadManagerDelegate : public content::DownloadManagerDelegate,
                                    public DownloadTargetDeterminerDelegate {
 public:
  explicit AtomDownloadManagerDelegate(content::DownloadManager* manager);
  virtual ~AtomDownloadManagerDelegate();

  bool GenerateFileHash() override;

  void OnDownloadTargetDetermined(
    int32_t download_id,
    const content::DownloadTargetCallback& callback,
    std::unique_ptr<DownloadTargetInfo> target_info);

  // content::DownloadManagerDelegate:
  void Shutdown() override;
  bool DetermineDownloadTarget(
      content::DownloadItem* download,
      const content::DownloadTargetCallback& callback) override;
  bool ShouldOpenDownload(
      content::DownloadItem* download,
      const content::DownloadOpenDelayedCallback& callback) override;
  void GetNextId(const content::DownloadIdCallback& callback) override;
  bool ShouldCompleteDownload(content::DownloadItem* item,
                              const base::Closure& complete_callback) override;

 protected:
  virtual safe_browsing::DownloadProtectionService*
      GetDownloadProtectionService();
  void CheckDownloadUrl(content::DownloadItem* download,
                            const base::FilePath& suggested_virtual_path,
                            const CheckDownloadUrlCallback& callback) override;
  void NotifyExtensions(content::DownloadItem* download,
                        const base::FilePath& suggested_virtual_path,
                        const NotifyExtensionsCallback& callback) override;
  void ReserveVirtualPath(
      content::DownloadItem* download,
      const base::FilePath& virtual_path,
      bool create_directory,
      DownloadPathReservationTracker::FilenameConflictAction conflict_action,
      const ReservedPathCallback& callback) override;

  void RequestConfirmation(content::DownloadItem* download,
                           const base::FilePath& suggested_virtual_path,
                           DownloadConfirmationReason reason,
                           const ConfirmationCallback& callback) override;
  void DetermineLocalPath(content::DownloadItem* download,
                          const base::FilePath& virtual_path,
                          const LocalPathCallback& callback) override;

  void GetFileMimeType(const base::FilePath& path,
                       const GetFileMimeTypeCallback& callback) override;

 private:
    bool IsDownloadReadyForCompletion(
        content::DownloadItem* item,
        const base::Closure& internal_complete_callback);

  void GetItemSavePath(content::DownloadItem* item, base::FilePath* path);

  bool GetExtension(content::DownloadItem* item,
                    const base::FilePath& target_path,
                    base::FilePath::StringType* extension);

  void OnDownloadItemSelected(const content::DownloadTargetCallback& callback,
                              std::unique_ptr<DownloadTargetInfo> target_info,
                              api::DownloadItem* download_item,
                              const std::vector<base::FilePath>& paths);

  void OnDownloadItemSelectionCancelled(
      const content::DownloadTargetCallback& callback,
      content::DownloadItem* item);

  void CheckClientDownloadDone(uint32_t download_id,
                               safe_browsing::DownloadCheckResult result);

  void ShouldCompleteDownloadInternal(
      uint32_t download_id,
      const base::Closure& user_complete_callback);

  content::DownloadManager* download_manager_;
  std::unique_ptr<DownloadPrefs> download_prefs_;
  base::WeakPtrFactory<AtomDownloadManagerDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomDownloadManagerDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_DOWNLOAD_MANAGER_DELEGATE_H_
