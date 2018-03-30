#include "chrome/browser/download/download_commands.h"

DownloadCommands::DownloadCommands(content::DownloadItem* download_item)
    : download_item_(download_item) {
}

DownloadCommands::~DownloadCommands() = default;

void DownloadCommands::ExecuteCommand(Command command) {
  return;
}
