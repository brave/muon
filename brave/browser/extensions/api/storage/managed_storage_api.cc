// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "brave/browser/extensions/api/storage/managed_storage_api.h"

#include "base/json/json_reader.h"
#include "extensions/browser/api/storage/storage_frontend.h"
#include "extensions/browser/event_router.h"
#include "extensions/common/api/storage.h"

namespace extensions {

ExtensionFunction::ResponseValue ManagedStorageSetFunction::RunWithStorage(
    ValueStore* storage) {
  base::DictionaryValue* input = NULL;
  if (!args_->GetDictionary(0, &input))
    return BadMessage();
  return UseWriteResult(storage->Set(ValueStore::DEFAULTS, *input));
}

ExtensionFunction::ResponseValue ManagedStorageSetFunction::UseWriteResult(
    ValueStore::WriteResult result) {
  if (result->status().ok() && !result->changes().empty()) {
    auto change_json = ValueStoreChange::ToJson(result->changes());
    // TODO(gdk): This is a temporary hack while the refactoring for
    // string-based event payloads is removed. http://crbug.com/136045
    std::unique_ptr<base::ListValue> args(new base::ListValue());
    std::unique_ptr<base::Value> changes = base::JSONReader::Read(change_json);
    DCHECK(changes);

    args->Append(std::move(changes));
    std::unique_ptr<Event> event(new Event(events::STORAGE_ON_CHANGED,
                                           "managedStorage.onChanged",
                                           std::move(args)));

    EventRouter::Get(browser_context())->DispatchEventToExtension(
        "", std::move(event));
  }

  return StorageStorageAreaSetFunction::UseWriteResult(std::move(result));
}

bool ManagedStorageSetFunction::PreRunValidation(std::string* error) {
  std::unique_ptr<base::Value> val(new base::Value("managed"));
  args_->Insert(0, std::move(val));
  return StorageStorageAreaSetFunction::PreRunValidation(error);
}

bool ManagedStorageGetFunction::PreRunValidation(std::string* error) {
  std::unique_ptr<base::Value> val(new base::Value("managed"));
  args_->Insert(0, std::move(val));
  return StorageStorageAreaGetFunction::PreRunValidation(error);
}

bool ManagedStorageRemoveFunction::PreRunValidation(std::string* error) {
  std::unique_ptr<base::Value> val(new base::Value("managed"));
  args_->Insert(0, std::move(val));
  return StorageStorageAreaRemoveFunction::PreRunValidation(error);
}

bool ManagedStorageClearFunction::PreRunValidation(std::string* error) {
  std::unique_ptr<base::Value> val(new base::Value("managed"));
  args_->Insert(0, std::move(val));
  return StorageStorageAreaClearFunction::PreRunValidation(error);
}


}  // namespace extensions
