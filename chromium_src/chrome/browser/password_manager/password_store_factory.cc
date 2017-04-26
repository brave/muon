// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_factory.h"

// #include <memory>
// #include <utility>

#include "build/build_config.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_default.h"
#include "components/password_manager/core/browser/password_store_factory_util.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

using password_manager::PasswordStore;

// static
scoped_refptr<PasswordStore> PasswordStoreFactory::GetForProfile(
    Profile* profile,
    ServiceAccessType access_type) {
  NOTREACHED();
  return scoped_refptr<PasswordStore>();
}

// static
PasswordStoreFactory* PasswordStoreFactory::GetInstance() {
  return base::Singleton<PasswordStoreFactory>::get();
}

PasswordStoreFactory::PasswordStoreFactory()
    : RefcountedBrowserContextKeyedServiceFactory(
          "PasswordStore",
          BrowserContextDependencyManager::GetInstance()) {
}

PasswordStoreFactory::~PasswordStoreFactory() {}

void PasswordStoreFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {}

scoped_refptr<RefcountedKeyedService>
PasswordStoreFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  NOTREACHED();
  return nullptr;
}

content::BrowserContext* PasswordStoreFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool PasswordStoreFactory::ServiceIsNULLWhileTesting() const {
  return true;
}


