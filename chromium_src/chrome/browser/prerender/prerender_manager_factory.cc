// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prerender/prerender_manager_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace prerender {

// static
PrerenderManager* PrerenderManagerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return nullptr;
}

// static
PrerenderManagerFactory* PrerenderManagerFactory::GetInstance() {
  return base::Singleton<PrerenderManagerFactory>::get();
}

PrerenderManagerFactory::PrerenderManagerFactory()
    : BrowserContextKeyedServiceFactory(
        "PrerenderManager",
        BrowserContextDependencyManager::GetInstance()) {
}

PrerenderManagerFactory::~PrerenderManagerFactory() {
}

KeyedService* PrerenderManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* browser_context) const {
  return nullptr;
}

content::BrowserContext* PrerenderManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

}  // namespace prerender
