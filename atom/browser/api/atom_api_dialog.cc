// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "atom/browser/api/atom_api_window.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/message_box.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/browser/extensions/api/file_system/file_entry_picker.h"
#include "chrome/common/chrome_paths.h"
#include "native_mate/dictionary.h"
#include "vendor/brightray/browser/inspectable_web_contents.h"

#include "atom/common/node_includes.h"

namespace file_dialog {

typedef base::Callback<void(
    bool result, const std::vector<base::FilePath>& paths)> DialogCallback;

struct DialogSettings {
  atom::NativeWindow* parent_window = nullptr;
  base::FilePath default_path;
  std::vector<std::vector<base::FilePath::StringType>> extensions;
  std::vector<base::string16> extension_description_overrides;
  ui::SelectFileDialog::Type type;
  bool include_all_files = true;
};
}  // namespace file_dialog

namespace mate {

template<>
struct Converter<ui::SelectFileDialog::Type> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     ui::SelectFileDialog::Type* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "select-folder") {
        *out = ui::SelectFileDialog::SELECT_FOLDER;
    } else if (type == "select-upload-folder") {
        *out = ui::SelectFileDialog::SELECT_UPLOAD_FOLDER;
    } else if (type == "select-saveas-file") {
        *out = ui::SelectFileDialog::SELECT_SAVEAS_FILE;
    } else if (type == "select-open-file") {
        *out = ui::SelectFileDialog::SELECT_OPEN_FILE;
    } else if (type == "select-open-multi-file") {
        *out = ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE;
    } else {
        *out = ui::SelectFileDialog::SELECT_NONE;
    }
    return true;
  }
};

template<>
struct Converter<file_dialog::DialogSettings> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     file_dialog::DialogSettings* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    dict.Get("window", &(out->parent_window));
    dict.Get("defaultPath", &(out->default_path));
    dict.Get("type", &(out->type));
    dict.Get("extensions", &(out->extensions));
    dict.Get("extensionDescriptionOverrides",
             &(out->extension_description_overrides));
    dict.Get("includeAllFiles", &(out->include_all_files));
    return true;
  }
};

}  // namespace mate

namespace {

void ShowMessageBox(int type,
                    const std::vector<std::string>& buttons,
                    int default_id,
                    int cancel_id,
                    int options,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const gfx::ImageSkia& icon,
                    atom::NativeWindow* window,
                    mate::Arguments* args) {
  v8::Local<v8::Value> peek = args->PeekNext();
  atom::MessageBoxCallback callback;
  if (mate::Converter<atom::MessageBoxCallback>::FromV8(args->isolate(),
                                                        peek,
                                                        &callback)) {
    atom::ShowMessageBox(window, (atom::MessageBoxType)type, buttons,
                         default_id, cancel_id, options, title,
                         message, detail, icon, callback);
  } else {
    int chosen = atom::ShowMessageBox(window, (atom::MessageBoxType)type,
                                      buttons, default_id, cancel_id,
                                      options, title, message, detail, icon);
    args->Return(chosen);
  }
}

void OnShowDialogSelected(const file_dialog::DialogCallback& callback,
                          const std::vector<base::FilePath>& paths) {
  DCHECK(!paths.empty());
  callback.Run(true, paths);
}

void OnShowDialogCancelled(const file_dialog::DialogCallback& callback) {
  std::vector<base::FilePath> files;
  files.push_back(base::FilePath());
  callback.Run(false, files);
}

void ShowDialog(const file_dialog::DialogSettings& settings,
                mate::Arguments* args) {
  v8::Local<v8::Value> peek = args->PeekNext();
  file_dialog::DialogCallback callback;
  if (!args->GetNext(&callback)) {
    args->ThrowError("`callback` is a required field");
    return;
  }

  ui::SelectFileDialog::FileTypeInfo file_type_info;
  for (size_t i = 0; i < settings.extensions.size(); ++i) {
    file_type_info.extensions.push_back(settings.extensions[i]);
  }
  if (file_type_info.extensions.empty()) {
    base::FilePath::StringType extension =
      settings.default_path.FinalExtension();
    if (!extension.empty()) {
      file_type_info.extensions.push_back(
        std::vector<base::FilePath::StringType>());
      extension.erase(extension.begin());  // drop the .
      file_type_info.extensions[0].push_back(extension);
    }
  }

  base::FilePath default_path = settings.default_path;
  if (default_path.empty()) {
    if (!PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS, &default_path)) {
      NOTREACHED();
    }
  }
  // This is only useful on platforms that support
  // DIR_DEFAULT_DOWNLOADS_SAFE.
  if (!PathService::Get(chrome::DIR_DEFAULT_DOWNLOADS_SAFE, &default_path)) {
    NOTREACHED();
  }
  file_type_info.include_all_files = settings.include_all_files;
  file_type_info.extension_description_overrides =
    settings.extension_description_overrides;
  file_type_info.allowed_paths =
    ui::SelectFileDialog::FileTypeInfo::NATIVE_OR_DRIVE_PATH;
  new extensions::FileEntryPicker(
    settings.parent_window->inspectable_web_contents()->GetWebContents(),
    default_path,
    file_type_info,
    settings.type,
    base::Bind(&OnShowDialogSelected, callback),
    base::Bind(&OnShowDialogCancelled, callback));
}

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("showMessageBox", &ShowMessageBox);
  dict.SetMethod("showErrorBox", &atom::ShowErrorBox);
  dict.SetMethod("showDialog", &ShowDialog);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_dialog, Initialize)
