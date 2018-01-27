// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <windows.h>  // windows.h must be included first

#include <atlbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wrl/client.h>

#include "atom/browser/ui/win/jump_list.h"
#include "atom/common/atom_version.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "base/base_paths.h"
#include "base/file_version_info.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"

namespace atom {

namespace {

const wchar_t kAppUserModelIDFormat[] = L"electron.app.$1";

BOOL CALLBACK WindowsEnumerationHandler(HWND hwnd, LPARAM param) {
  DWORD target_process_id = *reinterpret_cast<DWORD*>(param);
  DWORD process_id = 0;

  GetWindowThreadProcessId(hwnd, &process_id);
  if (process_id == target_process_id) {
    SetFocus(hwnd);
    return FALSE;
  }

  return TRUE;
}

bool GetProtocolLaunchPath(mate::Arguments* args, base::string16* exe) {
  // Executable Path
  if (!args || !args->GetNext(exe)) {
    base::FilePath path;
    if (!PathService::Get(base::FILE_EXE, &path)) {
      LOG(ERROR) << "Error getting app exe path";
      return false;
    }
    *exe = path.value();
    if (!args)
      return true;
  }

  // Read in optional args arg
  std::vector<base::string16> launch_args;
  if (args->GetNext(&launch_args) && !launch_args.empty())
    *exe = base::StringPrintf(L"\"%s\" %s \"%%1\"",
                              exe->c_str(),
                              base::JoinString(launch_args, L" ").c_str());
  else
    *exe = base::StringPrintf(L"\"%s\" \"%%1\"", exe->c_str());
  return true;
}

}  // namespace

void Browser::Focus() {
  // On Windows we just focus on the first window found for this process.
  DWORD pid = GetCurrentProcessId();
  EnumWindows(&WindowsEnumerationHandler, reinterpret_cast<LPARAM>(&pid));
}

void Browser::AddRecentDocument(const base::FilePath& path) {
  if (base::win::GetVersion() < base::win::VERSION_WIN7)
    return;

  CComPtr<IShellItem> item;
  HRESULT hr = SHCreateItemFromParsingName(
      path.value().c_str(), NULL, IID_PPV_ARGS(&item));
  if (SUCCEEDED(hr)) {
    SHARDAPPIDINFO info;
    info.psi = item;
    info.pszAppID = GetAppUserModelID();
    SHAddToRecentDocs(SHARD_APPIDINFO, &info);
  }
}

void Browser::ClearRecentDocuments() {
  CComPtr<IApplicationDestinations> destinations;
  if (FAILED(destinations.CoCreateInstance(CLSID_ApplicationDestinations,
                                           NULL, CLSCTX_INPROC_SERVER)))
    return;
  if (FAILED(destinations->SetAppID(GetAppUserModelID())))
    return;
  destinations->RemoveAllDestinations();
}

void Browser::SetAppUserModelID(const base::string16& name) {
  app_user_model_id_ = name;
  SetCurrentProcessExplicitAppUserModelID(app_user_model_id_.c_str());
}

bool Browser::SetUserTasks(const std::vector<UserTask>& tasks) {
  JumpList jump_list(GetAppUserModelID());
  if (!jump_list.Begin())
    return false;

  JumpListCategory category;
  category.type = JumpListCategory::Type::TASKS;
  category.items.reserve(tasks.size());
  JumpListItem item;
  item.type = JumpListItem::Type::TASK;
  for (const auto& task : tasks) {
    item.title = task.title;
    item.path = task.program;
    item.arguments = task.arguments;
    item.icon_path = task.icon_path;
    item.icon_index = task.icon_index;
    item.description = task.description;
    category.items.push_back(item);
  }

  jump_list.AppendCategory(category);
  return jump_list.Commit();
}

bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol,
                                            mate::Arguments* args) {
  if (protocol.empty())
    return false;

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  base::string16 keyPath = base::UTF8ToUTF16("Software\\Classes\\" + protocol);

  // Command Key
  base::string16 cmdPath = keyPath + L"\\shell\\open\\command";

  base::win::RegKey key;
  base::win::RegKey commandKey;
  if (FAILED(key.Open(root, keyPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't even exist, we can confirm that it is not set
    return true;

  if (FAILED(commandKey.Open(root, cmdPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't even exist, we can confirm that it is not set
    return true;

  base::string16 keyVal;
  if (FAILED(commandKey.ReadValue(L"", &keyVal)))
    // Default value not set, we can confirm that it is not set
    return true;

  base::string16 exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  if (keyVal == exe) {
    // Let's kill the key
    if (FAILED(key.DeleteKey(L"shell")))
      return false;

    return true;
  } else {
    return true;
  }
}

// Returns the target used as a activate parameter when opening the settings
// pointing to the page that is the most relevant to a user trying to change the
// default handler for |protocol|.
base::string16 GetTargetForDefaultAppsSettings(const wchar_t* protocol) {
  static const wchar_t kSystemSettingsDefaultAppsFormat[] =
      L"SystemSettings_DefaultApps_%ls";

  if (base::EqualsCaseInsensitiveASCII(protocol, L"http"))
    return base::StringPrintf(kSystemSettingsDefaultAppsFormat, L"Browser");
  if (base::EqualsCaseInsensitiveASCII(protocol, L"mailto"))
    return base::StringPrintf(kSystemSettingsDefaultAppsFormat, L"Email");
  return L"SettingsPageAppsDefaultsProtocolView";
}

// Launches the Windows 'settings' modern app with the 'default apps' view
// focused. This only works for Windows 8 and Windows 10. The appModelId
// looks arbitrary but it is the same in Win8 and Win10. There is no easy way to
// retrieve the appModelId from the registry.
bool LaunchDefaultAppsSettingsModernDialog(const wchar_t* protocol) {
  DCHECK(protocol);
  static const wchar_t kControlPanelAppModelId[] =
      L"windows.immersivecontrolpanel_cw5n1h2txyewy"
      L"!microsoft.windows.immersivecontrolpanel";

  Microsoft::WRL::ComPtr<IApplicationActivationManager> activator;
  HRESULT hr =
      ::CoCreateInstance(CLSID_ApplicationActivationManager, nullptr,
                         CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&activator));
  if (SUCCEEDED(hr)) {
    DWORD pid = 0;
    CoAllowSetForegroundWindow(activator.Get(), nullptr);
    hr = activator->ActivateApplication(kControlPanelAppModelId,
                                        L"page=SettingsPageAppsDefaults",
                                        AO_NONE, &pid);
    if (SUCCEEDED(hr)) {
      hr = activator->ActivateApplication(
          kControlPanelAppModelId,
          base::StringPrintf(L"page=SettingsPageAppsDefaults&target=%ls",
                             GetTargetForDefaultAppsSettings(protocol).c_str())
              .c_str(),
          AO_NONE, &pid);
    }
    if (SUCCEEDED(hr))
      return true;
  }
  return false;
}

bool Browser::SetAsDefaultProtocolClient(const std::string& protocol,
                                        mate::Arguments* args) {
  // HKEY_CLASSES_ROOT
  //    $PROTOCOL
  //       (Default) = "URL:$NAME"
  //       URL Protocol = ""
  //       shell
  //          open
  //             command
  //                (Default) = "$COMMAND" "%1"
  //
  // However, the "HKEY_CLASSES_ROOT" key can only be written by the
  // Administrator user. So, we instead write to "HKEY_CURRENT_USER\
  // Software\Classes", which is inherited by "HKEY_CLASSES_ROOT"
  // anyway, and can be written by unprivileged users.

  if (protocol.empty())
    return false;

  // TODO(Anthony): refactoring set default browser and set default protocol
  // client as separated APIs
  if (base::win::GetVersion() >= base::win::VERSION_WIN8 && protocol == "http")
    return LaunchDefaultAppsSettingsModernDialog(
      base::UTF8ToUTF16(protocol).c_str());

  base::string16 exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  base::string16 keyPath = base::UTF8ToUTF16("Software\\Classes\\" + protocol);
  base::string16 urlDecl = base::UTF8ToUTF16("URL:" + protocol);

  // Command Key
  base::string16 cmdPath = keyPath + L"\\shell\\open\\command";

  // Write information to registry
  base::win::RegKey key(root, keyPath.c_str(), KEY_ALL_ACCESS);
  if (FAILED(key.WriteValue(L"URL Protocol", L"")) ||
      FAILED(key.WriteValue(L"", urlDecl.c_str())))
    return false;

  base::win::RegKey commandKey(root, cmdPath.c_str(), KEY_ALL_ACCESS);
  if (FAILED(commandKey.WriteValue(L"", exe.c_str())))
    return false;

  // TODO(Anthony): Make it configurable
  base::string16 prog = L"BraveHTML";

  // choice path
  base::string16 choicePath = base::UTF8ToUTF16(
    "Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\"
    + protocol + "\\UserChoice");

  base::win::RegKey choiceKey(root, choicePath.c_str(), KEY_ALL_ACCESS);
  if (FAILED(choiceKey.WriteValue(L"Progid", prog.c_str())))
    return false;

  return true;
}

bool Browser::IsDefaultProtocolClient(const std::string& protocol,
                                      mate::Arguments* args) {
  if (protocol.empty())
    return false;

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  base::win::RegKey choiceKey;

  // TODO(Anthony): Make it configurable
  base::string16 prog = L"BraveHTML";

  // choice path
  base::string16 choicePath = base::UTF8ToUTF16(
    "Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\"
    + protocol + "\\UserChoice");

  if (FAILED(choiceKey.Open(root, choicePath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't exist, we can confirm that it is not set
    return false;

  base::string16 choiceVal;
  if (FAILED(choiceKey.ReadValue(L"Progid", &choiceVal)))
    // value not set, we can confirm that it is not set
    return false;

  // TODO(Anthony): refactoring is default browser and is default protocol
  // client as separated APIs
  if (base::win::GetVersion() >= base::win::VERSION_WIN8 &&
      (protocol == "http" || protocol == "https"))
    return choiceVal == prog;

  base::string16 exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  base::string16 keyPath = base::UTF8ToUTF16("Software\\Classes\\" + protocol);

  // Command Key
  base::string16 cmdPath = keyPath + L"\\shell\\open\\command";

  base::win::RegKey key;
  base::win::RegKey commandKey;
  if (FAILED(key.Open(root, keyPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't exist, we can confirm that it is not set
    return false;

  if (FAILED(commandKey.Open(root, cmdPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't exist, we can confirm that it is not set
    return false;

  base::string16 keyVal;
  if (FAILED(commandKey.ReadValue(L"", &keyVal)))
    // Default value not set, we can confirm that it is not set
    return false;

  // Default value is the same as current file path and
  // user choice in url associations should be same as prog
  return keyVal == exe && choiceVal == prog;
}

bool Browser::SetBadgeCount(int count) {
  return false;
}

void Browser::SetLoginItemSettings(LoginItemSettings settings) {
  base::string16 keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  base::win::RegKey key(HKEY_CURRENT_USER, keyPath.c_str(), KEY_ALL_ACCESS);

  if (settings.open_at_login) {
    base::FilePath path;
    if (PathService::Get(base::FILE_EXE, &path)) {
      base::string16 exePath(path.value());
      key.WriteValue(GetAppUserModelID(), exePath.c_str());
    }
  } else {
    key.DeleteValue(GetAppUserModelID());
  }
}

Browser::LoginItemSettings Browser::GetLoginItemSettings() {
  LoginItemSettings settings;
  base::string16 keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  base::win::RegKey key(HKEY_CURRENT_USER, keyPath.c_str(), KEY_ALL_ACCESS);
  base::string16 keyVal;

  if (!FAILED(key.ReadValue(GetAppUserModelID(), &keyVal))) {
    base::FilePath path;
    if (PathService::Get(base::FILE_EXE, &path)) {
      base::string16 exePath(path.value());
      settings.open_at_login = keyVal == exePath;
    }
  }

  return settings;
}


PCWSTR Browser::GetAppUserModelID() {
  if (app_user_model_id_.empty()) {
    SetAppUserModelID(base::ReplaceStringPlaceholders(
        kAppUserModelIDFormat, base::UTF8ToUTF16(GetName()), nullptr));
  }

  return app_user_model_id_.c_str();
}

std::string Browser::GetExecutableFileVersion() const {
  base::FilePath path;
  if (PathService::Get(base::FILE_EXE, &path)) {
    std::unique_ptr<FileVersionInfo> version_info(
        FileVersionInfo::CreateFileVersionInfo(path));
    return base::UTF16ToUTF8(version_info->product_version());
  }

  return ATOM_VERSION_STRING;
}

std::string Browser::GetExecutableFileProductName() const {
  base::FilePath path;
  if (PathService::Get(base::FILE_EXE, &path)) {
    std::unique_ptr<FileVersionInfo> version_info(
        FileVersionInfo::CreateFileVersionInfo(path));
    return base::UTF16ToUTF8(version_info->product_name());
  }

  return ATOM_PRODUCT_NAME;
}

}  // namespace atom
