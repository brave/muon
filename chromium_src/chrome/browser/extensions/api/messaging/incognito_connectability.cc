// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/incognito_connectability.h"

#include "base/lazy_instance.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/extension.h"
#include "ui/base/l10n/l10n_util.h"

namespace extensions {

IncognitoConnectability::IncognitoConnectability(
    content::BrowserContext* context)
    : weak_factory_(this) {
  CHECK(context->IsOffTheRecord());
}

IncognitoConnectability::~IncognitoConnectability() {
}

// static
IncognitoConnectability* IncognitoConnectability::Get(
    content::BrowserContext* context) {
  return BrowserContextKeyedAPIFactory<IncognitoConnectability>::Get(context);
}

void IncognitoConnectability::Query(
    const Extension* extension,
    content::WebContents* web_contents,
    const GURL& url,
    const base::Callback<void(bool)>& callback) {
  callback.Run(false);
}

static base::LazyInstance<
    BrowserContextKeyedAPIFactory<IncognitoConnectability> >::DestructorAtExit
      g_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<IncognitoConnectability>*
IncognitoConnectability::GetFactoryInstance() {
  return g_factory.Pointer();
}

IncognitoConnectability::TabContext::TabContext() : infobar(nullptr) {
}

IncognitoConnectability::TabContext::TabContext(const TabContext& other) =
    default;

IncognitoConnectability::TabContext::~TabContext() {
}

}  // namespace extensions
