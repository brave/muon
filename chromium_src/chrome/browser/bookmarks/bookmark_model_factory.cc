// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bookmarks/bookmark_model_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/browser/startup_task_runner_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "components/undo/bookmark_undo_service.h"
#include "content/public/browser/browser_thread.h"

using bookmarks::BookmarkModel;

// static
BookmarkModel* BookmarkModelFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return nullptr;
}

// static
BookmarkModel* BookmarkModelFactory::GetForBrowserContextIfExists(
    content::BrowserContext* context) {
  return nullptr;
}

// static
BookmarkModelFactory* BookmarkModelFactory::GetInstance() {
  return base::Singleton<BookmarkModelFactory>::get();
}

BookmarkModelFactory::BookmarkModelFactory()
    : BrowserContextKeyedServiceFactory(
        "BookmarkModel",
        BrowserContextDependencyManager::GetInstance()) {
}

BookmarkModelFactory::~BookmarkModelFactory() {
}

KeyedService* BookmarkModelFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return nullptr;
}

void BookmarkModelFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
}

content::BrowserContext* BookmarkModelFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool BookmarkModelFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
