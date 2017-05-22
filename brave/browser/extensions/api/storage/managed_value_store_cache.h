// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_EXTENSIONS_API_STORAGE_MANAGED_VALUE_STORE_CACHE_H_
#define BRAVE_BROWSER_EXTENSIONS_API_STORAGE_MANAGED_VALUE_STORE_CACHE_H_

#include <map>
#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "extensions/browser/api/storage/settings_observer.h"
#include "extensions/browser/api/storage/value_store_cache.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class ValueStoreFactory;
class ReadOnlyValueStore;

// A ValueStoreCache that manages a ReadOnlyValueStore for each extension that
// uses the storage.managed namespace.
class ManagedValueStoreCache : public ValueStoreCache {
 public:
  // |factory| is used to create databases for the ReadOnlyValueStores.
  // |observers| is the list of SettingsObservers to notify when a ValueStore
  // changes.
  ManagedValueStoreCache(content::BrowserContext* context,
                         const scoped_refptr<ValueStoreFactory>& factory,
                         const scoped_refptr<SettingsObserverList>& observers);
  ~ManagedValueStoreCache() override;

 private:
  // ValueStoreCache implementation:
  void RunWithValueStoreForExtension(
      const StorageCallback& callback,
      scoped_refptr<const Extension> extension) override;
  void DeleteStorageSoon(const std::string& extension_id) override;

  // Returns true if a backing store has been created for |extension_id|.
  bool HasStore(const std::string& extension_id) const;
  ValueStore* GetStoreFor(const std::string& extension_id);
  ReadOnlyValueStore* GetStoreForImpl(const std::string& extension_id);

  // These live on the FILE thread.
  scoped_refptr<ValueStoreFactory> storage_factory_;
  scoped_refptr<SettingsObserverList> observers_;

  content::BrowserContext* browser_context_;

  // All the ReadOnlyValueStores live on the FILE thread, and |store_map_|
  // can be accessed only on the FILE thread as well.
  std::map<std::string, std::unique_ptr<ReadOnlyValueStore>> store_map_;

  DISALLOW_COPY_AND_ASSIGN(ManagedValueStoreCache);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_STORAGE_MANAGED_VALUE_STORE_CACHE_H_
