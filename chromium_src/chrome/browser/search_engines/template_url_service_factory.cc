// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search_engines/template_url_service_factory.h"

#include <string>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/template_url_service.h"

// static
TemplateURLService* TemplateURLServiceFactory::GetForProfile(Profile* profile) {
  NOTREACHED();
  return nullptr;
}

// static
TemplateURLServiceFactory* TemplateURLServiceFactory::GetInstance() {
  return base::Singleton<TemplateURLServiceFactory>::get();
}

// static
std::unique_ptr<KeyedService> TemplateURLServiceFactory::BuildInstanceFor(
    content::BrowserContext* context) {
  NOTREACHED();
  return std::unique_ptr<TemplateURLService>();
}

TemplateURLServiceFactory::TemplateURLServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "TemplateURLServiceFactory",
        BrowserContextDependencyManager::GetInstance()) {
}

TemplateURLServiceFactory::~TemplateURLServiceFactory() {}

KeyedService* TemplateURLServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  NOTREACHED();
  return nullptr;
}

void TemplateURLServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
}

content::BrowserContext* TemplateURLServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  NOTREACHED();
  return nullptr;
}

bool TemplateURLServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
