// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_DOWNLOAD_MANAGER_DELEGATE_H_
#define ATOM_BROWSER_ATOM_DOWNLOAD_MANAGER_DELEGATE_H_

#include <string>
#include <vector>

#include "atom/browser/api/atom_api_download_item.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/download/download_path_reservation_tracker.h"
#include "content/public/browser/download_manager_delegate.h"

namespace content {
class DownloadManager;
}

namespace atom {

class AtomDownloadManagerDelegate : public content::DownloadManagerDelegate {
 public:
  explicit AtomDownloadManagerDelegate(content::DownloadManager* manager);
  virtual ~AtomDownloadManagerDelegate();

  void OnDownloadPathGenerated(
    int32_t download_id,
    const content::DownloadTargetCallback& callback,
    PathValidationResult result,
    const base::FilePath& target_path);

  // content::DownloadManagerDelegate:
  void Shutdown() override;
  bool DetermineDownloadTarget(
      download::DownloadItem* download,
      const content::DownloadTargetCallback& callback) override;
  bool ShouldOpenDownload(
      download::DownloadItem* download,
      const content::DownloadOpenDelayedCallback& callback) override;
  void GetNextId(const content::DownloadIdCallback& callback) override;

 private:
  // Get the save path set on the associated api::DownloadItem object
  void GetItemSavePath(download::DownloadItem* item, base::FilePath* path);

  bool GetExtension(download::DownloadItem* item,
                    const base::FilePath& target_path,
                    base::FilePath::StringType* extension);

  void OnDownloadItemSelected(const content::DownloadTargetCallback& callback,
                              api::DownloadItem* download_item,
                              const std::vector<base::FilePath>& paths);

  void OnDownloadItemSelectionCancelled(
      const content::DownloadTargetCallback& callback,
      download::DownloadItem* item);


  content::DownloadManager* download_manager_;
  base::WeakPtrFactory<AtomDownloadManagerDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AtomDownloadManagerDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_DOWNLOAD_MANAGER_DELEGATE_H_
