// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_importer.h"

#include <iostream>
#include <memory>
#include <string>

#include "atom/browser/importer/profile_writer.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/importer/brave_external_process_importer_host.h"
#include "chrome/browser/importer/importer_list.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/dictionary.h"

using content::BrowserThread;

namespace atom {

namespace api {

Importer::Importer(v8::Isolate* isolate)
  : importer_host_(NULL),
  import_did_succeed_(false) {
    Init(isolate);
    profile_writer_ = new ProfileWriter(NULL);
}

Importer::~Importer() {
  if (importer_host_)
    importer_host_->set_observer(NULL);
}

void Importer::InitializeImporter() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  importer_list_.reset(new ImporterList());
  importer_list_->DetectSourceProfiles(
    brave::BraveContentBrowserClient::Get()->GetApplicationLocale(),
    true,  // include_interactive_profiles?
    base::Bind(&Importer::InitializePage, base::Unretained(this)));
  profile_writer_->Initialize(this);
}

void Importer::ImportData(const base::DictionaryValue& data) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::string index;
  bool history, favorites, passwords, search, homepage, autofill_form_data,
       cookies;

  int browser_index = -1;
  if (!data.GetString("index", &index) ||
      !base::StringToInt(index, &browser_index)) {
    NOTREACHED();
    return;
  }

  uint16_t selected_items = importer::NONE;
  if (data.GetBoolean("history", &history) && history == true) {
    selected_items |= importer::HISTORY;
  }
  if (data.GetBoolean("favorites", &favorites) && favorites == true) {
    selected_items |= importer::FAVORITES;
  }
  if (data.GetBoolean("passwords", &passwords) && passwords == true) {
    selected_items |= importer::PASSWORDS;
  }
  if (data.GetBoolean("search", &search) && search == true) {
    selected_items |= importer::SEARCH_ENGINES;
  }
  if (data.GetBoolean("homepage", &homepage) && homepage == true) {
    selected_items |= importer::HOME_PAGE;
  }
  if (data.GetBoolean("autofill-autofill_form_data", &autofill_form_data) &&
      autofill_form_data == true) {
    selected_items |= importer::AUTOFILL_FORM_DATA;
  }
  if (data.GetBoolean("cookies", &cookies) && cookies == true) {
    selected_items |= importer::COOKIES;
  }

  const importer::SourceProfile& source_profile =
      importer_list_->GetSourceProfileAt(browser_index);
  uint16_t supported_items = source_profile.services_supported;

  uint16_t imported_items = (selected_items & supported_items);
  if (imported_items) {
    StartImport(source_profile, imported_items);
  } else {
    LOG(WARNING) << "There were no settings to import from '"
        << source_profile.importer_name << "'.";
  }
}

void Importer::ImportHTML(const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  importer::SourceProfile source_profile;
  source_profile.importer_type = importer::TYPE_BOOKMARKS_FILE;
  source_profile.source_path = path;

  StartImport(source_profile, importer::FAVORITES);
}

void Importer::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t imported_items) {
  // If another import is already ongoing, let it finish silently.
  if (importer_host_)
    importer_host_->set_observer(NULL);

  import_did_succeed_ = false;

  importer_host_ = new BraveExternalProcessImporterHost();
  importer_host_->set_observer(this);
  importer_host_->StartImportSettings(source_profile, NULL,
                                      imported_items,
                                      profile_writer_.get());
}

void Importer::InitializePage() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::ListValue browser_profiles;
  for (size_t i = 0; i < importer_list_->count(); ++i) {
    const importer::SourceProfile& source_profile =
        importer_list_->GetSourceProfileAt(i);
    uint16_t browser_services = source_profile.services_supported;

    base::DictionaryValue* browser_profile = new base::DictionaryValue();

    browser_profile->SetString("name", source_profile.importer_name);
    browser_profile->SetInteger("type", source_profile.importer_type);
    browser_profile->SetInteger("index", i);
    browser_profile->SetBoolean("history",
        (browser_services & importer::HISTORY) != 0);
    browser_profile->SetBoolean("favorites",
        (browser_services & importer::FAVORITES) != 0);
    browser_profile->SetBoolean("passwords",
        (browser_services & importer::PASSWORDS) != 0);
    browser_profile->SetBoolean("search",
        (browser_services & importer::SEARCH_ENGINES) != 0);
    browser_profile->SetBoolean("homepage",
        (browser_services & importer::HOME_PAGE) != 0);
    browser_profile->SetBoolean("autofill-form-data",
        (browser_services & importer::AUTOFILL_FORM_DATA) != 0);
    browser_profile->SetBoolean("cookies",
        (browser_services & importer::COOKIES) != 0);

    browser_profiles.Append(std::unique_ptr<base::DictionaryValue>(
                                browser_profile));
  }
  Emit("update-supported-browsers", browser_profiles);
}

// static
mate::Handle<Importer> Importer::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new Importer(isolate));
}

void Importer::ImportStarted() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void Importer::ImportItemStarted(importer::ImportItem item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // TODO(csilv): show progress detail in the web view.
}

void Importer::ImportItemEnded(importer::ImportItem item) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // TODO(csilv): show progress detail in the web view.
  import_did_succeed_ = true;
}

void Importer::ImportEnded() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  importer_host_->set_observer(NULL);
  importer_host_ = NULL;

  if (import_did_succeed_) {
    Emit("import-success");
  } else {
    Emit("import-dismiss");
  }
}

// static
void Importer::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Importer"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("initialize", &Importer::InitializeImporter)
      .SetMethod("importData", &Importer::ImportData)
      .SetMethod("importHTML", &Importer::ImportHTML);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::Importer;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("importer", Importer::Create(isolate));
  dict.Set("Importer", Importer::GetConstructor(isolate)->GetFunction());
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_importer, Initialize);
