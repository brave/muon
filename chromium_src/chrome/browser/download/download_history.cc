#include "chrome/browser/download/download_history.h"

#include "components/download/public/common/download_item.h"

bool DownloadHistory::WasRestoredFromHistory(
    const download::DownloadItem* download) const {
  return true;
}
