// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/extensions/api/storage/managed_value_store_cache.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/storage/settings_observer.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/value_store/value_store.h"
#include "extensions/browser/value_store/value_store_change.h"
#include "extensions/browser/value_store/value_store_factory.h"
#include "extensions/common/api/storage.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"

using content::BrowserContext;
using content::BrowserThread;

namespace extensions {
class ExtensionRegistry;

namespace storage = api::storage;

namespace {

// Only extension settings are stored in the managed namespace - not apps.
const ValueStoreFactory::ModelType kManagedModelType =
    ValueStoreFactory::ModelType::EXTENSION;

ValueStore::Status ReadOnlyError() {
  return ValueStore::Status(ValueStore::READ_ONLY,
                            "This is a read-only store.");
}

}  // namespace

class ReadOnlyValueStore : public ValueStore {
 public:
  ReadOnlyValueStore(
    const std::string& extension_id,
    const scoped_refptr<SettingsObserverList>& observers,
    std::unique_ptr<ValueStore> delegate)
    : extension_id_(extension_id),
      observers_(observers),
      delegate_(std::move(delegate)) {}

  ~ReadOnlyValueStore() override {};

  ValueStore* delegate() { return delegate_.get(); }

  // Clears all the stored data and deletes the database.
  void DeleteStorage() {
    // This is called from our owner, indicating that storage for this extension
    // should be removed.
    delegate_->Clear();
  }

  // ValueStore implementation:
  size_t GetBytesInUse(const std::string& key) override {
    // LeveldbValueStore doesn't implement this; and the underlying database
    // isn't acccessible to the extension in any case; from the extension's
    // perspective this is a read-only store.
    return 0;
  }

  size_t GetBytesInUse(const std::vector<std::string>& keys) override {
    // See note above.
    return 0;
  }

  size_t GetBytesInUse() override {
    // See note above.
    return 0;
  }

  ValueStore::ReadResult Get(const std::string& key) override {
    return delegate_->Get(key);
  }

  ValueStore::ReadResult Get(const std::vector<std::string>& keys) override {
    return delegate_->Get(keys);
  }

  ValueStore::ReadResult Get() override {
    return delegate_->Get();
  }

  ValueStore::WriteResult Set(
      WriteOptions options,
      const std::string& key,
      const base::Value& value) override {
    return MakeWriteResult(ReadOnlyError());
  }

  ValueStore::WriteResult Set(
      WriteOptions options, const base::DictionaryValue& settings) override {
    return MakeWriteResult(ReadOnlyError());
  }

  ValueStore::WriteResult Remove(const std::string& key) override {
    return MakeWriteResult(ReadOnlyError());
  }

  ValueStore::WriteResult Remove(
      const std::vector<std::string>& keys) override {
    return MakeWriteResult(ReadOnlyError());
  }

  ValueStore::WriteResult Clear() override {
    return MakeWriteResult(ReadOnlyError());
  }

 private:
  std::string extension_id_;
  scoped_refptr<SettingsObserverList> observers_;
  std::unique_ptr<ValueStore> delegate_;

  DISALLOW_COPY_AND_ASSIGN(ReadOnlyValueStore);
};

ManagedValueStoreCache::ManagedValueStoreCache(
    BrowserContext* context,
    const scoped_refptr<ValueStoreFactory>& factory,
    const scoped_refptr<SettingsObserverList>& observers)
    : storage_factory_(factory),
      observers_(observers),
      browser_context_(context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

ManagedValueStoreCache::~ManagedValueStoreCache() {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  // Delete the ReadOnlyValueStores on FILE.
  store_map_.clear();
}

void ManagedValueStoreCache::RunWithValueStoreForExtension(
    const StorageCallback& callback,
    scoped_refptr<const Extension> extension) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  callback.Run(GetStoreFor(extension->id()));
}

void ManagedValueStoreCache::DeleteStorageSoon(
    const std::string& extension_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  // It's possible that the store exists, but hasn't been loaded yet
  // (because the extension is unloaded, for example). Open the database to
  // clear it if it exists.
  if (!HasStore(extension_id))
    return;
  GetStoreForImpl(extension_id)->DeleteStorage();
  store_map_.erase(extension_id);
}

ValueStore* ManagedValueStoreCache::GetStoreFor(
    const std::string& extension_id) {
  auto value_store = GetStoreForImpl(extension_id);

  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(browser_context_)
          ->GetInstalledExtension(extension_id);

  if (extension && extension->was_installed_by_oem()) {
    return value_store->delegate();
  } else {
    return value_store;
  }
}

ReadOnlyValueStore* ManagedValueStoreCache::GetStoreForImpl(
    const std::string& extension_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);

  auto it = store_map_.find(extension_id);
  if (it != store_map_.end())
    return it->second.get();

  std::unique_ptr<ReadOnlyValueStore> store(new ReadOnlyValueStore(
      extension_id, observers_,
      storage_factory_->CreateSettingsStore(settings_namespace::MANAGED,
                                            kManagedModelType, extension_id)));
  ReadOnlyValueStore* raw_store = store.get();
  store_map_[extension_id] = std::move(store);

  return raw_store;
}

bool ManagedValueStoreCache::HasStore(const std::string& extension_id) const {
  // Note: Currently only manage extensions (not apps).
  return storage_factory_->HasSettings(settings_namespace::MANAGED,
                                       kManagedModelType, extension_id);
}

}  // namespace extensions
