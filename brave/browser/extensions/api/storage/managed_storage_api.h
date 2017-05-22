// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_EXTENSIONS_API_STORAGE_MANAGED_STORAGE_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_STORAGE_MANAGED_STORAGE_API_H_

#include <string>

#include "extensions/browser/api/storage/storage_api.h"

#include "extensions/browser/api/storage/storage_frontend.h"

namespace extensions {

class ManagedStorageGetFunction :
    public StorageStorageAreaGetFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("managedStorage.get", STORAGE_GET)

 protected:
  bool PreRunValidation(std::string* error) override;
};

class ManagedStorageSetFunction :
    public StorageStorageAreaSetFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("managedStorage.set", STORAGE_SET)

 protected:
  bool PreRunValidation(std::string* error) override;
  ResponseValue RunWithStorage(ValueStore* storage) override;
  ResponseValue UseWriteResult(ValueStore::WriteResult result);
};

class ManagedStorageRemoveFunction :
    public StorageStorageAreaRemoveFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("managedStorage.remove", STORAGE_REMOVE)

 protected:
  bool PreRunValidation(std::string* error) override;
};

class ManagedStorageClearFunction :
    public StorageStorageAreaClearFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("managedStorage.clear", STORAGE_CLEAR)

 protected:
  bool PreRunValidation(std::string* error) override;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_STORAGE_MANAGED_STORAGE_API_H_
