#include "chrome/browser/download/download_history.h"

#include "content/public/browser/download_item.h"

bool DownloadHistory::WasRestoredFromHistory(
    const content::DownloadItem* download) const {
  return true;
}
