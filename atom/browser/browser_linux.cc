// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "atom/browser/native_window.h"
#include "atom/browser/window_list.h"
#include "atom/common/atom_version.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/process/launch.h"
#include "brightray/common/application_info.h"
#include "chrome/browser/ui/libgtkui/unity_service.h"

const char kXdgSettings[] = "xdg-settings";
const char kXdgSettingsDefaultBrowser[] = "default-web-browser";
const char kXdgSettingsDefaultSchemeHandler[] = "default-url-scheme-handler";

// Value returned by xdg-settings if it can't understand our request.
const int EXIT_XDG_SETTINGS_SYNTAX_ERROR = 1;

// Helper to launch xdg scripts. We don't want them to ask any questions on the
// terminal etc. The function returns true if the utility launches and exits
// cleanly, in which case |exit_code| returns the utility's exit code.
bool LaunchXdgUtility(const std::vector<std::string>& argv, int* exit_code) {
  // xdg-settings internally runs xdg-mime, which uses mv to move newly-created
  // files on top of originals after making changes to them. In the event that
  // the original files are owned by another user (e.g. root, which can happen
  // if they are updated within sudo), mv will prompt the user to confirm if
  // standard input is a terminal (otherwise it just does it). So make sure it's
  // not, to avoid locking everything up waiting for mv.
  *exit_code = EXIT_FAILURE;
  int devnull = open("/dev/null", O_RDONLY);
  if (devnull < 0)
    return false;

  base::LaunchOptions options;
  options.fds_to_remap.push_back(std::make_pair(devnull, STDIN_FILENO));
  base::Process process = base::LaunchProcess(argv, options);
  close(devnull);
  if (!process.IsValid())
    return false;
  return process.WaitForExit(exit_code);
}

namespace atom {

void Browser::Focus() {
  // Focus on the first visible window.
  WindowList* list = WindowList::GetInstance();
  for (WindowList::iterator iter = list->begin(); iter != list->end(); ++iter) {
    NativeWindow* window = *iter;
    if (window->IsVisible()) {
      window->Focus(true);
      break;
    }
  }
}

void Browser::AddRecentDocument(const base::FilePath& path) {
}

void Browser::ClearRecentDocuments() {
}

void Browser::SetAppUserModelID(const base::string16& name) {
}

bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol,
                                            mate::Arguments* args) {
  return false;
}

// We delegate the difficulty of setting the default browser and default url
// scheme handler in Linux desktop environments to an xdg utility, xdg-settings.

// When calling this script we first try to use the script on PATH.

// If |protocol| is empty this function sets Brave as the default browser,
// otherwise it sets Brave as the default handler application for |protocol|.
bool Browser::SetAsDefaultProtocolClient(const std::string& protocol,
                                         mate::Arguments* args) {
  std::string desktop_name;
  if (args && !args->GetNext(&desktop_name)) {
    args->ThrowError("`desktop name` is a required field");
    return false;
  }

  std::unique_ptr<base::Environment> env(base::Environment::Create());

  std::vector<std::string> argv;
  argv.push_back(kXdgSettings);
  argv.push_back("set");
  if (protocol.empty()) {
    argv.push_back(kXdgSettingsDefaultBrowser);
  } else {
    argv.push_back(kXdgSettingsDefaultSchemeHandler);
    argv.push_back(protocol);
  }
  argv.push_back(desktop_name.c_str());

  int exit_code;
  bool ran_ok = LaunchXdgUtility(argv, &exit_code);
  if (ran_ok && exit_code == EXIT_XDG_SETTINGS_SYNTAX_ERROR) {
    return false;
  }

  return ran_ok && exit_code == EXIT_SUCCESS;
}

// If |protocol| is empty this function checks if Brave is the default browser,
// otherwise it checks if Brave is the default handler application for
// |protocol|.
bool Browser::IsDefaultProtocolClient(const std::string& protocol,
                                      mate::Arguments* args) {
  std::string desktop_name;
  if (args && !args->GetNext(&desktop_name)) {
    args->ThrowError("`desktop name` is a required field");
    return false;
  }

  base::AssertBlockingAllowed();

  std::unique_ptr<base::Environment> env(base::Environment::Create());

  std::vector<std::string> argv;
  argv.push_back(kXdgSettings);
  argv.push_back("check");
  if (protocol.empty()) {
    argv.push_back(kXdgSettingsDefaultBrowser);
  } else {
    argv.push_back(kXdgSettingsDefaultSchemeHandler);
    argv.push_back(protocol);
  }

  argv.push_back(desktop_name.c_str());

  std::string reply;
  int success_code;
  bool ran_ok = base::GetAppOutputWithExitCode(base::CommandLine(argv), &reply,
                                               &success_code);
  if (ran_ok && success_code == EXIT_XDG_SETTINGS_SYNTAX_ERROR) {
    return false;
  }

  if (!ran_ok || success_code != EXIT_SUCCESS) {
    // xdg-settings failed: we can't determine or set the default browser.
    return false;
  }

  // Allow any reply that starts with "yes".
  return (reply.find("yes") == 0) ? true : false;
}

bool Browser::SetBadgeCount(int count) {
  if (IsUnityRunning()) {
    unity::SetDownloadCount(count);
    badge_count_ = count;
    return true;
  } else {
    return false;
  }
}

void Browser::SetLoginItemSettings(LoginItemSettings settings) {
}

Browser::LoginItemSettings Browser::GetLoginItemSettings() {
  return LoginItemSettings();
}

std::string Browser::GetExecutableFileVersion() const {
  return brightray::GetApplicationVersion();
}

std::string Browser::GetExecutableFileProductName() const {
  return brightray::GetApplicationName();
}

bool Browser::IsUnityRunning() {
  return unity::IsRunning();
}

}  // namespace atom
